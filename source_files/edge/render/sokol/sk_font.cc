
#include "ddf_font.h"
#include "ddf_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "hu_font.h"
#include "i_defs_gl.h"
#include "im_data.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_texgl.h"
#include "sk_local.h"
#include "stb_truetype.h"
#include "w_files.h"
#include "w_wad.h"
#include "sk_state.h"

extern ImageData *ReadAsEpiBlock(Image *rim);

void Font::LoadPatches()
{
    // range of characters
    int              first = 9999;
    int              last  = 0;
    const Image    **images;
    const Image     *missing;
    const FontPatch *pat;

    // determine full range
    for (pat = definition_->patches_; pat; pat = pat->next)
    {
        if (pat->char1 < first)
            first = pat->char1;

        if (pat->char2 > last)
            last = pat->char2;
    }

    int total = last - first + 1;

    EPI_ASSERT(definition_->patches_);
    EPI_ASSERT(total >= 1);

    images = new const Image *[total];
    memset(images, 0, sizeof(const Image *) * total);

    // Atlas Stuff
    std::unordered_map<int, ImageData *> patch_data;
    std::vector<ImageData *>             temp_imdata;

    missing                   = definition_->missing_patch_ != ""
                                    ? ImageLookup(definition_->missing_patch_.c_str(), kImageNamespaceGraphic,
                                                  kImageLookupFont | kImageLookupNull)
                                    : nullptr;
    ImageData *missing_imdata = nullptr;

    if (missing)
    {
        ImageData *tmp_img = ReadAsEpiBlock((Image *)(missing));
        if (tmp_img->depth_ == 1)
        {
            ImageData *rgb_img = RGBFromPalettised(tmp_img, (const uint8_t *)&playpal_data[0], missing->opacity_);
            delete tmp_img;
            missing_imdata = rgb_img;
        }
        else
            missing_imdata = tmp_img;
        missing_imdata->offset_x_ = missing->offset_x_;
        missing_imdata->offset_y_ = missing->offset_y_;
        missing_imdata->scale_x_  = missing->scale_x_;
        missing_imdata->scale_y_  = missing->scale_y_;
    }

    // First pass, add the images that are good
    for (pat = definition_->patches_; pat; pat = pat->next)
    {
        // patch name
        char pname[40];

        EPI_ASSERT(strlen(pat->patch1.c_str()) < 36);
        strcpy(pname, pat->patch1.c_str());

        for (int ch = pat->char1; ch <= pat->char2; ch++, BumpPatchName(pname))
        {
            int idx = ch - first;
            EPI_ASSERT(0 <= idx && idx < total);

            images[idx] = ImageLookup(pname, kImageNamespaceGraphic, kImageLookupFont | kImageLookupNull);

            if (images[idx])
            {
                ImageData *tmp_img = ReadAsEpiBlock((Image *)(images[idx]));
                if (tmp_img->depth_ == 1)
                {
                    ImageData *rgb_img =
                        RGBFromPalettised(tmp_img, (const uint8_t *)&playpal_data[0], images[idx]->opacity_);
                    delete tmp_img;
                    tmp_img = rgb_img;
                }
                tmp_img->offset_x_ = images[idx]->offset_x_;
                tmp_img->offset_y_ = images[idx]->offset_y_;
                tmp_img->scale_x_  = images[idx]->scale_x_;
                tmp_img->scale_y_  = images[idx]->scale_y_;
                patch_data.try_emplace(kCP437UnicodeValues[(uint8_t)ch], tmp_img);
                temp_imdata.push_back(tmp_img);
            }
        }
    }

    // Second pass to try lower->uppercase fallbacks, or failing that add the
    // missing image (if present)
    for (int ch = 0; ch < 256; ch++)
    {
        if (!patch_data.count(kCP437UnicodeValues[(uint8_t)ch]))
        {
            if ('a' <= ch && ch <= 'z' && patch_data.count(kCP437UnicodeValues[(uint8_t)(epi::ToUpperASCII(ch))]))
                patch_data.try_emplace(kCP437UnicodeValues[(uint8_t)ch],
                                       patch_data.at(kCP437UnicodeValues[(uint8_t)(epi::ToUpperASCII(ch))]));
            else if (missing_imdata)
                patch_data.try_emplace(kCP437UnicodeValues[(uint8_t)ch], missing_imdata);
        }
    }

    ImageAtlas *atlas = PackImages(patch_data);
    for (auto patch : temp_imdata)
        delete patch;
    delete missing_imdata;
    if (atlas)
    {
        // Uncomment this to save the generated atlas. Note: will be inverted.
        /*std::string atlas_png = epi::PathAppend(home_directory,
        epi::StringFormat("atlas_%s.png", definition_->name.c_str())); if
        (epi::FileExists(atlas_png)) epi::FS_Remove(atlas_png);
        SavePNG(atlas_png, atlas->data);*/
        patch_font_cache_.atlas_rectangles = atlas->rectangles_;
        /*/
        glGenTextures(1, &patch_font_cache_.atlas_texture_id);
        global_render_state->BindTexture(patch_font_cache_.atlas_texture_id);
        global_render_state->TextureMinFilter(GL_NEAREST);
        global_render_state->TextureMagFilter(GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->data_->width_, atlas->data_->height_, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, atlas->data_->pixels_);
        */

        /*
        glGenTextures(1, &patch_font_cache_.atlas_smoothed_texture_id);
        global_render_state->BindTexture(patch_font_cache_.atlas_smoothed_texture_id);
        global_render_state->TextureMinFilter(GL_LINEAR);
        global_render_state->TextureMagFilter(GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->data_->width_, atlas->data_->height_, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, atlas->data_->pixels_);
        */

        SokolRenderState *fixme = (SokolRenderState *)global_render_state;

        sg_sampler_desc sdesc = {0};

        sdesc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        sdesc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;

        // filtering
        sdesc.mag_filter    = SG_FILTER_NEAREST;
        sdesc.min_filter    = SG_FILTER_NEAREST;
        sdesc.mipmap_filter = SG_FILTER_NEAREST;

        sg_image_desc img_desc = {0};
        img_desc.width         = atlas->data_->width_;
        img_desc.height        = atlas->data_->height_;

        img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

        sg_image_data img_data     = {0};
        img_data.subimage[0]->ptr  = atlas->data_->pixels_;
        img_data.subimage[0]->size = atlas->data_->width_ * atlas->data_->height_ * 4;

        img_desc.num_mipmaps = 1;
        img_desc.data        = img_data;

        sg_image image                     = sg_make_image(&img_desc);
        fixme->RegisterImage(image.id, &sdesc);
        patch_font_cache_.atlas_texture_id = image.id;        

        // Dupe as we don't have the sampler changes in yet
        image                                       = sg_make_image(&img_desc);
        fixme->RegisterImage(image.id, &sdesc);
        patch_font_cache_.atlas_smoothed_texture_id = image.id;

        atlas->data_->Whiten();
        img_data.subimage[0]->ptr  = atlas->data_->pixels_;
        img_data.subimage[0]->size = atlas->data_->width_ * atlas->data_->height_ * 4;

        image                                       = sg_make_image(&img_desc);
        fixme->RegisterImage(image.id, &sdesc);
        patch_font_cache_.atlas_whitened_texture_id = image.id;

        image                                                = sg_make_image(&img_desc);
        fixme->RegisterImage(image.id, &sdesc);
        patch_font_cache_.atlas_whitened_smoothed_texture_id = image.id;

        /*
        glGenTextures(1, &patch_font_cache_.atlas_whitened_texture_id);
        global_render_state->BindTexture(patch_font_cache_.atlas_whitened_texture_id);
        global_render_state->TextureMinFilter(GL_NEAREST);
        global_render_state->TextureMagFilter(GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->data_->width_, atlas->data_->height_, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, atlas->data_->pixels_);

        glGenTextures(1, &patch_font_cache_.atlas_whitened_smoothed_texture_id);
        global_render_state->BindTexture(patch_font_cache_.atlas_whitened_smoothed_texture_id);
        global_render_state->TextureMinFilter(GL_LINEAR);
        global_render_state->TextureMagFilter(GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlas->data_->width_, atlas->data_->height_, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, atlas->data_->pixels_);
        */
        delete atlas;
    }
    else
        FatalError("Failed to create atlas for patch font %s!\n", definition_->name_.c_str());

    if (patch_font_cache_.atlas_rectangles.empty())
    {
        LogWarning("Font [%s] has no loaded patches !\n", definition_->name_.c_str());
        patch_font_cache_.width = patch_font_cache_.height = 7;
        return;
    }

    ImageAtlasRectangle Nom;

    if (patch_font_cache_.atlas_rectangles.count(kCP437UnicodeValues[(uint8_t)('M')]))
        Nom = patch_font_cache_.atlas_rectangles.at(kCP437UnicodeValues[(uint8_t)('M')]);
    else if (patch_font_cache_.atlas_rectangles.count(kCP437UnicodeValues[(uint8_t)('m')]))
        Nom = patch_font_cache_.atlas_rectangles.at(kCP437UnicodeValues[(uint8_t)('m')]);
    else if (patch_font_cache_.atlas_rectangles.count(kCP437UnicodeValues[(uint8_t)('0')]))
        Nom = patch_font_cache_.atlas_rectangles.at(kCP437UnicodeValues[(uint8_t)('0')]);
    else // backup plan: just use first patch found
        Nom = patch_font_cache_.atlas_rectangles.begin()->second;

    if (definition_->default_size_ > 0.0)
    {
        patch_font_cache_.height = definition_->default_size_;
        patch_font_cache_.width  = definition_->default_size_ * (Nom.image_width / Nom.image_height);
        patch_font_cache_.ratio  = patch_font_cache_.width / patch_font_cache_.height;
    }
    else
    {
        patch_font_cache_.width  = Nom.image_width;
        patch_font_cache_.height = Nom.image_height;
        patch_font_cache_.ratio  = Nom.image_width / Nom.image_height;
    }
    spacing_ = definition_->spacing_;
}
