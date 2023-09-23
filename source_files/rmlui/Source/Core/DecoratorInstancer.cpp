/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "../../Include/RmlUi/Core/DecoratorInstancer.h"
#include "../../Include/RmlUi/Core/StyleSheet.h"

namespace Rml {

DecoratorInstancer::DecoratorInstancer() : properties(10, 10) {}

DecoratorInstancer::~DecoratorInstancer() {}

const PropertySpecification& DecoratorInstancer::GetPropertySpecification() const
{
	return properties;
}

PropertyDefinition& DecoratorInstancer::RegisterProperty(const String& property_name, const String& default_value)
{
	return properties.RegisterProperty(property_name, default_value, false, false);
}

ShorthandId DecoratorInstancer::RegisterShorthand(const String& shorthand_name, const String& property_names, ShorthandType type)
{
	return properties.RegisterShorthand(shorthand_name, property_names, type);
}

const Sprite* DecoratorInstancerInterface::GetSprite(const String& name) const
{
	return style_sheet.GetSprite(name);
}

Texture DecoratorInstancerInterface::GetTexture(const String& filename) const
{
	Texture texture;

	if (!property_source)
	{
		Log::Message(Log::LT_WARNING, "Texture name '%s' in decorator could not be loaded, no property source available.", filename.c_str());
		return texture;
	}

	texture.Set(filename, property_source->path);

	return texture;
}

} // namespace Rml
