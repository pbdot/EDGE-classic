//----------------------------------------------------------------------------
//  EDGE Heads-up-display Font code
//----------------------------------------------------------------------------
//
//  Copyright (c) 2004-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "hu_font.h"

#include "ddf_font.h"
#include "ddf_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "i_defs_gl.h"
#include "im_data.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_texgl.h"
#include "stb_truetype.h"
#include "w_files.h"
#include "w_wad.h"

// TODO: Don't leak sokol code here
#ifdef EDGE_SOKOL
#include "render/sokol/sk_local.h"
#include "render/sokol/sk_state.h"
#endif

static constexpr uint8_t kDummyCharacterWidth = 8;

extern ImageData *ReadAsEpiBlock(Image *rim);

// all the fonts that's fit to print
FontContainer hud_fonts;

int current_font_size;

static constexpr int truetype_scaling_font_sizes[3]   = {12, 24, 48};
static constexpr int truetype_scaling_bitmap_sizes[3] = {512, 1024, 2048};

Font::Font(FontDefinition *definition) : definition_(definition)
{
    font_image_      = nullptr;
    truetype_info_   = nullptr;
    truetype_buffer_ = nullptr;
    EPI_CLEAR_MEMORY(truetype_kerning_scale_, float, 3);
    EPI_CLEAR_MEMORY(truetype_reference_yshift_, float, 3);
    EPI_CLEAR_MEMORY(truetype_reference_height_, float, 3);
}

Font::~Font()
{
}

void Font::BumpPatchName(char *name)
{
    // loops to increment the 10s (100s, etc) digit
    for (char *s = name + strlen(name) - 1; s >= name; s--)
    {
        // only handle digits and letters
        if (!epi::IsAlphanumericASCII(*s))
            break;

        if (*s == '9')
        {
            *s = '0';
            continue;
        }
        if (*s == 'Z')
        {
            *s = 'A';
            continue;
        }
        if (*s == 'z')
        {
            *s = 'a';
            continue;
        }

        (*s) += 1;
        break;
    }
}

void Font::LoadFontImage()
{
    if (!font_image_)
    {
        if (!definition_->image_name_.empty())
            font_image_ = ImageLookup(definition_->image_name_.c_str(), kImageNamespaceGraphic,
                                      kImageLookupExact | kImageLookupNull);
        else
            FatalError("LoadFontImage: nullptr image name provided for font %s!", definition_->name_.c_str());
        if (!font_image_)
            FatalError("LoadFontImage: Image %s not found for font %s!", definition_->image_name_.c_str(),
                       definition_->name_.c_str());
        int char_height = font_image_->actual_height_ / 16;
        int char_width  = font_image_->actual_width_ / 16;
        image_character_height_ =
            (definition_->default_size_ == 0.0 ? char_height : definition_->default_size_) * font_image_->scale_y_;
        image_character_width_ =
            (definition_->default_size_ == 0.0 ? char_width : definition_->default_size_) * font_image_->scale_x_;
        image_monospace_width_ = 0;
        spacing_               = definition_->spacing_;
        // Determine individual character widths and ratios
        individual_char_widths_ = new float[256];
        individual_char_ratios_ = new float[256];
        ImageData *char_data    = ReadAsEpiBlock((Image *)font_image_);
        for (int i = 0; i < 256; i++)
        {
            int px = i % 16;
            int py = 15 - i / 16;
            individual_char_widths_[i] =
                char_data->ImageCharacterWidth(px * char_width, py * char_height, px * char_width + char_width,
                                               py * char_height + char_height) *
                font_image_->scale_x_;
            if (definition_->default_size_ > 0.0)
                individual_char_widths_[i] *= (definition_->default_size_ / char_width);
            if (individual_char_widths_[i] > image_monospace_width_)
                image_monospace_width_ = individual_char_widths_[i];
            individual_char_ratios_[i] = individual_char_widths_[i] / image_character_height_;
        }
        delete char_data;
    }
}

void Font::LoadFontTTF()
{
    if (!truetype_buffer_)
    {
        if (definition_->truetype_name_.empty())
        {
            FatalError("LoadFontTTF: No TTF file/lump name provided for font %s!", definition_->name_.c_str());
        }

        for (size_t i = 0; i < hud_fonts.size(); i++)
        {
            if (epi::StringCaseCompareASCII(hud_fonts[i]->definition_->truetype_name_, definition_->truetype_name_) ==
                0)
            {
                if (hud_fonts[i]->truetype_buffer_)
                    truetype_buffer_ = hud_fonts[i]->truetype_buffer_;
                if (hud_fonts[i]->truetype_info_)
                    truetype_info_ = hud_fonts[i]->truetype_info_;
            }
        }

        if (!truetype_buffer_)
        {
            epi::File *F;

            if (!epi::GetExtension(definition_->truetype_name_).empty()) // check for pack file
                F = OpenFileFromPack(definition_->truetype_name_);
            else
                F = LoadLumpAsFile(CheckLumpNumberForName(definition_->truetype_name_.c_str()));

            if (!F)
                FatalError("LoadFontTTF: '%s' not found for font %s.\n", definition_->truetype_name_.c_str(),
                           definition_->name_.c_str());

            truetype_buffer_ = F->LoadIntoMemory();

            delete F;
        }

        if (!truetype_info_)
        {
            truetype_info_ = new stbtt_fontinfo;
            if (!stbtt_InitFont(truetype_info_, truetype_buffer_, 0))
                FatalError("LoadFontTTF: Could not initialize font %s.\n", definition_->name_.c_str());
        }

        TrueTypeCharacter ref;

        ref.glyph_index = 0;

        char ch = 0;

        if (stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)('M')]) > 0)
        {
            ch              = 'M';
            ref.glyph_index = stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)ch]);
        }
        else if (stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)('O')]) > 0)
        {
            ch              = 'O';
            ref.glyph_index = stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)ch]);
        }
        else if (stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)('W')]) > 0)
        {
            ch              = 'W';
            ref.glyph_index = stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)ch]);
        }
        else
        {
            for (char c = 32; c < 127; c++)
            {
                if (stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)(c)]) > 0)
                {
                    ch              = c;
                    ref.glyph_index = stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)ch]);
                    break;
                }
            }
        }

        if (ref.glyph_index == 0)
            FatalError("LoadFontTTF: No suitable characters in font %s.\n", definition_->name_.c_str());

        for (int i = 0; i < 3; i++)
        {
            truetype_atlas_[i]                                   = new stbtt_pack_range;
            truetype_atlas_[i]->first_unicode_codepoint_in_range = 0;
            truetype_atlas_[i]->array_of_unicode_codepoints      = (int *)kCP437UnicodeValues;
            truetype_atlas_[i]->font_size                        = truetype_scaling_font_sizes[i];
            truetype_atlas_[i]->num_chars                        = 256;
            truetype_atlas_[i]->chardata_for_range               = new stbtt_packedchar[256];

            if (definition_->default_size_ == 0.0)
                definition_->default_size_ = 7.0f;

            truetype_kerning_scale_[i] = stbtt_ScaleForPixelHeight(truetype_info_, definition_->default_size_);

            unsigned char *temp_bitmap =
                new unsigned char[truetype_scaling_bitmap_sizes[i] * truetype_scaling_bitmap_sizes[i]];

            stbtt_pack_context *spc = new stbtt_pack_context;
            stbtt_PackBegin(spc, temp_bitmap, truetype_scaling_bitmap_sizes[i], truetype_scaling_bitmap_sizes[i], 0, 1,
                            nullptr);
            stbtt_PackSetOversampling(spc, 2, 2);
            stbtt_PackFontRanges(spc, truetype_buffer_, 0, truetype_atlas_[i], 1);
            stbtt_PackEnd(spc);
#ifndef EDGE_SOKOL
            global_render_state->GenTextures(1, &truetype_texture_id_[i]);
            global_render_state->BindTexture(truetype_texture_id_[i]);
            global_render_state->TextureMinFilter(GL_NEAREST);
            global_render_state->TextureMagFilter(GL_NEAREST);
            global_render_state->TexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, truetype_scaling_bitmap_sizes[i],
                                            truetype_scaling_bitmap_sizes[i], 0, GL_ALPHA, GL_UNSIGNED_BYTE,
                                            temp_bitmap);
            global_render_state->GenTextures(1, &truetype_smoothed_texture_id_[i]);
            global_render_state->BindTexture(truetype_smoothed_texture_id_[i]);
            global_render_state->TextureMinFilter(GL_LINEAR);
            global_render_state->TextureMagFilter(GL_LINEAR);
            global_render_state->TexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, truetype_scaling_bitmap_sizes[i],
                                            truetype_scaling_bitmap_sizes[i], 0, GL_ALPHA, GL_UNSIGNED_BYTE,
                                            temp_bitmap);
#else

            SokolRenderState *fixme = (SokolRenderState *)global_render_state;

            // Default sampler
            sg_sampler_desc sdesc = {0};

            sdesc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
            sdesc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;

            // filtering
            sdesc.mag_filter    = SG_FILTER_NEAREST;
            sdesc.min_filter    = SG_FILTER_NEAREST;
            sdesc.mipmap_filter = SG_FILTER_NEAREST;

            sg_image_data img_data       = {0};
            img_data.subimage[0][0].ptr  = temp_bitmap;
            img_data.subimage[0][0].size = truetype_scaling_bitmap_sizes[i] * truetype_scaling_bitmap_sizes[i];

            sg_image_desc img_desc = {0};
            img_desc.width         = truetype_scaling_bitmap_sizes[i];
            img_desc.height        = truetype_scaling_bitmap_sizes[i];

            img_desc.pixel_format = SG_PIXELFORMAT_R8;

            img_desc.num_mipmaps = 1;
            img_desc.data        = img_data;

            sg_image image          = sg_make_image(&img_desc);
            truetype_texture_id_[i] = image.id;
            fixme->RegisterImage(image.id, &sdesc);


            // Dupe :/  Can this this with samplers and no dupe
            image                            = sg_make_image(&img_desc);
            truetype_smoothed_texture_id_[i] = image.id;
            fixme->RegisterImage(image.id, &sdesc);

#endif
            delete[] temp_bitmap;
            float x               = 0.0f;
            float y               = 0.0f;
            float ascent          = 0.0f;
            float descent         = 0.0f;
            float linegap         = 0.0f;
            ref.character_quad[i] = new stbtt_aligned_quad;
            stbtt_GetPackedQuad(truetype_atlas_[i]->chardata_for_range, truetype_scaling_bitmap_sizes[i],
                                truetype_scaling_bitmap_sizes[i], (uint8_t)ch, &x, &y, ref.character_quad[i], 0);
            stbtt_GetScaledFontVMetrics(truetype_buffer_, 0, truetype_scaling_font_sizes[i], &ascent, &descent,
                                        &linegap);
            ref.width[i] = (ref.character_quad[i]->x1 - ref.character_quad[i]->x0) *
                           (definition_->default_size_ / truetype_scaling_font_sizes[i]);
            ref.height[i] = (ref.character_quad[i]->y1 - ref.character_quad[i]->y0) *
                            (definition_->default_size_ / truetype_scaling_font_sizes[i]);
            truetype_character_width_[i] = ref.width[i];
            truetype_character_height_[i] =
                (ascent - descent) * (definition_->default_size_ / truetype_scaling_font_sizes[i]);
            ref.y_shift[i] =
                (truetype_character_height_[i] - ref.height[i]) +
                (ref.character_quad[i]->y1 * (definition_->default_size_ / truetype_scaling_font_sizes[i]));
            truetype_reference_yshift_[i] = ref.y_shift[i];
            truetype_reference_height_[i] = ref.height[i];
        }
        truetype_glyph_map_.try_emplace((uint8_t)ch, ref);
        spacing_ = definition_->spacing_ + 0.5; // + 0.5 for at least a minimal buffer
                                                // between letters by default
    }
}

void Font::Load()
{
    switch (definition_->type_)
    {
    case kFontTypePatch:
        LoadPatches();
        break;

    case kFontTypeImage:
        LoadFontImage();
        break;

    case kFontTypeTrueType:
        LoadFontTTF();
        break;

    default:
        FatalError("Coding error, unknown font type %d\n", definition_->type_);
        break; /* NOT REACHED */
    }
}

float Font::NominalWidth() const
{
    if (definition_->type_ == kFontTypeImage)
        return image_character_width_ + spacing_;

    if (definition_->type_ == kFontTypePatch)
        return patch_font_cache_.width + spacing_;

    if (definition_->type_ == kFontTypeTrueType)
        return truetype_character_width_[current_font_size] + spacing_;

    FatalError("font_c::NominalWidth : unknown FONT type %d\n", definition_->type_);
    return 1; /* NOT REACHED */
}

float Font::NominalHeight() const
{
    if (definition_->type_ == kFontTypeImage)
        return image_character_height_;

    if (definition_->type_ == kFontTypePatch)
        return patch_font_cache_.height;

    if (definition_->type_ == kFontTypeTrueType)
        return truetype_character_height_[current_font_size];

    FatalError("font_c::NominalHeight : unknown FONT type %d\n", definition_->type_);
    return 1; /* NOT REACHED */
}

const Image *Font::CharImage(char ch) const
{
    if (definition_->type_ == kFontTypeImage)
        return font_image_;

    if (definition_->type_ == kFontTypeTrueType)
    {
        if (truetype_glyph_map_.find((uint8_t)ch) != truetype_glyph_map_.end())
            // Create or return dummy image
            return ImageLookup("FONT_DUMMY_IMAGE", kImageNamespaceGraphic, kImageLookupFont);
        else
            return nullptr;
    }

    EPI_ASSERT(definition_->type_ == kFontTypePatch);

    if (ch == ' ')
        return nullptr;

    if (patch_font_cache_.atlas_rectangles.count(kCP437UnicodeValues[(uint8_t)ch]))
        return ImageLookup("FONT_DUMMY_IMAGE", kImageNamespaceGraphic, kImageLookupFont);
    else
        return nullptr;
}

float Font::CharRatio(char ch)
{
    EPI_ASSERT(definition_->type_ == kFontTypeImage);

    if (ch == ' ')
        return 0.4f;
    else
        return individual_char_ratios_[(uint8_t)ch];
}

//
// Returns the width of the IBM cp437 char in the font.
//
float Font::CharWidth(char ch)
{
    if (definition_->type_ == kFontTypeImage)
    {
        if (ch == ' ')
            return image_character_width_ * 2 / 5 + spacing_;
        else
            return individual_char_widths_[(uint8_t)ch] + spacing_;
    }

    if (definition_->type_ == kFontTypeTrueType)
    {
        auto find_glyph = truetype_glyph_map_.find((uint8_t)ch);
        if (find_glyph != truetype_glyph_map_.end())
            return (find_glyph->second.width[current_font_size] + spacing_) * pixel_aspect_ratio.f_;
        else
        {
            TrueTypeCharacter character;
            for (int i = 0; i < 3; i++)
            {
                character.character_quad[i] = new stbtt_aligned_quad;
                float x                     = 0.0f;
                float y                     = 0.0f;
                stbtt_GetPackedQuad(truetype_atlas_[i]->chardata_for_range, truetype_scaling_bitmap_sizes[i],
                                    truetype_scaling_bitmap_sizes[i], (uint8_t)ch, &x, &y, character.character_quad[i],
                                    0);
                if (ch == ' ')
                    character.width[i] = truetype_character_width_[i] * 3 / 5;
                else
                    character.width[i] = (character.character_quad[i]->x1 - character.character_quad[i]->x0) *
                                         (definition_->default_size_ / truetype_scaling_font_sizes[i]);
                character.height[i] = (character.character_quad[i]->y1 - character.character_quad[i]->y0) *
                                      (definition_->default_size_ / truetype_scaling_font_sizes[i]);
                character.y_shift[i] =
                    (truetype_character_height_[i] - character.height[i]) +
                    (character.character_quad[i]->y1 * (definition_->default_size_ / truetype_scaling_font_sizes[i]));
            }
            character.glyph_index = stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)ch]);
            truetype_glyph_map_.try_emplace((uint8_t)ch, character);
            return (character.width[current_font_size] + spacing_) * pixel_aspect_ratio.f_;
        }
    }

    EPI_ASSERT(definition_->type_ == kFontTypePatch);

    if (ch == ' ')
        return patch_font_cache_.width * 3 / 5 + spacing_;

    if (!patch_font_cache_.atlas_rectangles.count(kCP437UnicodeValues[(uint8_t)ch]))
        return kDummyCharacterWidth;

    ImageAtlasRectangle rect = patch_font_cache_.atlas_rectangles.at(kCP437UnicodeValues[(uint8_t)ch]);

    if (definition_->default_size_ > 0.0)
        return (definition_->default_size_ * ((float)rect.image_width) / rect.image_height) + spacing_;
    else
        return rect.image_width + spacing_;
}

//
// Returns the maximum number of characters which can fit within pixel_w
// pixels.  The string may not contain any newline characters.
//
int Font::MaxFit(int pixel_w, const char *str)
{
    int         w = 0;
    const char *s;

    // just add one char at a time until it gets too wide or the string ends.
    for (s = str; *s; s++)
    {
        w += CharWidth(*s);

        if (w > pixel_w)
        {
            // if no character could fit, an infinite loop would probably start,
            // so it's better to just imagine that one character fits.
            if (s == str)
                s++;

            break;
        }
    }

    // extra spaces at the end of the line can always be added
    while (*s == ' ')
        s++;

    return s - str;
}

//
// Get glyph index for TTF Character. If character hasn't been cached yet, cache
// it.
//
int Font::GetGlyphIndex(char ch)
{
    EPI_ASSERT(definition_->type_ == kFontTypeTrueType);

    auto find_glyph = truetype_glyph_map_.find((uint8_t)ch);
    if (find_glyph != truetype_glyph_map_.end())
        return find_glyph->second.glyph_index;
    else
    {
        TrueTypeCharacter character;
        for (int i = 0; i < 3; i++)
        {
            character.character_quad[i] = new stbtt_aligned_quad;
            float x                     = 0.0f;
            float y                     = 0.0f;
            stbtt_GetPackedQuad(truetype_atlas_[i]->chardata_for_range, truetype_scaling_bitmap_sizes[i],
                                truetype_scaling_bitmap_sizes[i], (uint8_t)ch, &x, &y, character.character_quad[i], 0);
            if (ch == ' ')
                character.width[i] = truetype_character_width_[i] * 3 / 5;
            else
                character.width[i] = (character.character_quad[i]->x1 - character.character_quad[i]->x0) *
                                     (definition_->default_size_ / truetype_scaling_font_sizes[i]);
            character.height[i] = (character.character_quad[i]->y1 - character.character_quad[i]->y0) *
                                  (definition_->default_size_ / truetype_scaling_font_sizes[i]);
            character.y_shift[i] =
                (truetype_character_height_[i] - character.height[i]) +
                (character.character_quad[i]->y1 * (definition_->default_size_ / truetype_scaling_font_sizes[i]));
        }
        character.glyph_index = stbtt_FindGlyphIndex(truetype_info_, kCP437UnicodeValues[(uint8_t)ch]);
        truetype_glyph_map_.try_emplace((uint8_t)ch, character);
        return character.glyph_index;
    }
}

//
// Find string width from hu_font chars.  The string may not contain
// any newline characters.
//
float Font::StringWidth(const char *str)
{
    float w = 0;

    if (!str)
        return 0;

    std::string_view width_checker = str;

    for (size_t i = 0; i < width_checker.size(); i++)
    {
        w += CharWidth(width_checker[i]);
        if (definition_->type_ == kFontTypeTrueType && i + 1 < width_checker.size())
            w += stbtt_GetGlyphKernAdvance(truetype_info_, GetGlyphIndex(width_checker[i]),
                                           GetGlyphIndex(width_checker[i + 1])) *
                 truetype_kerning_scale_[current_font_size];
    }

    return w;
}

//
// Find number of lines in string.
//
int StringLines(std::string_view str)
{
    int                         slines = 1;
    std::string_view::size_type oldpos = 0;
    std::string_view::size_type pos    = 0;

    while (pos != std::string_view::npos)
    {
        pos = str.find('\n', oldpos);
        if (pos != std::string_view::npos)
        {
            slines++;
            oldpos = pos + 1;
        }
    }

    return slines;
}

//----------------------------------------------------------------------------
//  FontContainer class
//----------------------------------------------------------------------------

// Never returns nullptr.
//
Font *FontContainer::Lookup(FontDefinition *definition)
{
    EPI_ASSERT(definition);

    for (std::vector<Font *>::iterator iter = begin(), iter_end = end(); iter != iter_end; iter++)
    {
        Font *f = *iter;

        if (definition == f->definition_)
            return f;
    }

    Font *new_f = new Font(definition);

    new_f->Load();
    push_back(new_f);

    return new_f;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
