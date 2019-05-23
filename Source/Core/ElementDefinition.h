/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
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

#ifndef ROCKETCOREELEMENTDEFINITION_H
#define ROCKETCOREELEMENTDEFINITION_H

#include "../../Include/Rocket/Core/Dictionary.h"
#include "../../Include/Rocket/Core/ReferenceCountable.h"
#include <map>
#include "../../Include/Rocket/Core/FontEffect.h"
#include "StyleSheetNode.h"

namespace Rocket {
namespace Core {

class Decorator;
class FontEffect;

// Defines for the optimised version of the pseudo-class properties (note the difference from the
// PseudoClassPropertyMap defined in StyleSheetNode.h ... bit clumsy). Here the properties are stored as a list
// of definitions against each property name in specificity-order, along with the pseudo-class requirements for each
// one. This makes it much more straight-forward to query at run-time.
typedef std::pair< StringList, Property > PseudoClassProperty;
typedef std::vector< PseudoClassProperty > PseudoClassPropertyList;
typedef SmallUnorderedMap< String, PseudoClassPropertyList > PseudoClassPropertyDictionary;

typedef SmallUnorderedMap< String, Decorator* > DecoratorMap;
typedef std::map< StringList, DecoratorMap > PseudoClassDecoratorMap;

/**
	@author Peter Curry
 */

class ElementDefinition : public ReferenceCountable
{
public:
	enum PseudoClassVolatility
	{
		STABLE,					// pseudo-class has no volatility
		FONT_VOLATILE,			// pseudo-class may impact on font effects
		STRUCTURE_VOLATILE		// pseudo-class may impact on definitions of child elements
	};

	ElementDefinition();
	virtual ~ElementDefinition();

	/// Initialises the element definition from a list of style sheet nodes.
	void Initialise(const std::vector< const StyleSheetNode* >& style_sheet_nodes, const PseudoClassList& volatile_pseudo_classes, bool structurally_volatile);

	/// Returns a specific property from the element definition's base properties.
	/// @param[in] name The name of the property to return.
	/// @param[in] pseudo_classes The pseudo-classes currently active on the calling element.
	/// @return The property defined against the give name, or NULL if no such property was found.
	const Property* GetProperty(const String& name, const PseudoClassList& pseudo_classes) const;

	/// Returns the list of properties this element definition defines for an element with the given set of
	/// pseudo-classes.
	/// @param[out] property_names The list to store the defined properties in.
	/// @param[in] pseudo_classes The pseudo-classes defined on the querying element.
	void GetDefinedProperties(PropertyNameList& property_names, const PseudoClassList& pseudo_classes) const;
	/// Returns the list of properties this element definition has explicit definitions for involving the given
	/// pseudo-class.
	/// @param[out] property_names The list of store the newly defined / undefined properties in.
	/// @param[in] pseudo_classes The list of pseudo-classes currently set on the element (post-change).
	/// @param[in] pseudo_class The pseudo-class that was just activated or deactivated.
	void GetDefinedProperties(PropertyNameList& property_names, const PseudoClassList& pseudo_classes, const String& pseudo_class) const;

	/// Iterates over the properties in the definition.
	/// @param[inout] index Index of the property to fetch. This is incremented to the next valid index after the fetch.
	/// @param[in] pseudo_classes The pseudo-classes defined on the querying element.
	/// @param[out] property_pseudo_classes The pseudo-classes the property is defined by.
	/// @param[out] property_name The name of the property at the specified index.
	/// @param[out] property The property at the specified index.
	/// @return True if a property was successfully fetched.
	bool IterateProperties(int& index, const PseudoClassList& pseudo_classes, String& property_name, const Property*& property, const PseudoClassList** property_pseudo_classes = nullptr) const;

	/// Returns the list of the element definition's instanced decorators in the default state.
	/// @return The list of instanced decorators.
	const DecoratorMap& GetDecorators() const;
	/// Returns the map of pseudo-class names to overriding instanced decorators.
	/// @return The map of the overriding decorators for each pseudo-class.
	const PseudoClassDecoratorMap& GetPseudoClassDecorators() const;

	/// Appends this definition's font effects (appropriately for the given pseudo classes) into a
	/// provided map of effects.
	/// @param[out] font_effects The outgoing map of font effects.
	/// @param[in] pseudo_classes Pseudo-classes active on the querying element.
	void GetFontEffects(FontEffectMap& font_effects, const PseudoClassList& pseudo_classes) const;

	/// Returns the volatility of a pseudo-class.
	/// @param[in] pseudo_class The name of the pseudo-class to check for volatility.
	/// @return The volatility of the pseudo-class.
	PseudoClassVolatility GetPseudoClassVolatility(const String& pseudo_class) const;

	/// Returns true if this definition is built from nodes using structural selectors, and therefore is reliant on
	/// siblings remaining stable.
	/// @return True if this definition is structurally volatile.
	bool IsStructurallyVolatile() const;


	class Iterator {
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = std::pair<const String&, const Property&>;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::input_iterator_tag;

		using PropertyIt = PropertyMap::const_iterator;
		using PseudoIt = PseudoClassPropertyDictionary::const_iterator;

		Iterator() : pseudo_classes(nullptr) {}
		Iterator(const StringList& pseudo_classes, PropertyIt it_properties, PseudoIt it_pseudo_class_properties, PropertyIt it_properties_end, PseudoIt it_pseudo_class_properties_end)
			: pseudo_classes(&pseudo_classes), it_properties(it_properties), it_pseudo_class_properties(it_pseudo_class_properties), it_properties_end(it_properties_end), it_pseudo_class_properties_end(it_pseudo_class_properties_end)
		{
			proceed_to_next_valid();
		}
		Iterator& operator++()
		{
			// The iteration proceeds as follows:
			//  1. Iterate over all the default properties of the element (with no pseudo classes)
			//  2. Iterate over each pseudo class that has a definition for this property,
			//      testing each one to see if it matches the currently set pseudo classes.
			if (it_properties != it_properties_end)
			{
				++it_properties;
				proceed_to_next_valid();
				return *this;
			}
			++i_pseudo_class;
			proceed_to_next_valid();
			return *this;
		}
		bool operator==(Iterator other) const { return pseudo_classes == other.pseudo_classes && it_properties == other.it_properties && it_pseudo_class_properties == other.it_pseudo_class_properties && i_pseudo_class == other.i_pseudo_class; }
		bool operator!=(Iterator other) const { return !(*this == other); }
		value_type operator*() const {
			if (it_properties != it_properties_end)
				return { it_properties->first, it_properties->second };
			return { it_pseudo_class_properties->first,  it_pseudo_class_properties->second[i_pseudo_class].second };
		}

		// Return the list of pseudo classes which defines the current property, possibly null
		const PseudoClassList* pseudo_class_list() const
		{
			if (it_properties != it_properties_end)
				return nullptr;
			return &it_pseudo_class_properties->second[i_pseudo_class].first;
		}

	private:
		const StringList* pseudo_classes;
		PropertyIt it_properties, it_properties_end;
		PseudoIt it_pseudo_class_properties, it_pseudo_class_properties_end;
		size_t i_pseudo_class = 0;

		void proceed_to_next_valid()
		{
			if (it_properties == it_properties_end)
			{
				// Iterate over all the pseudo classes and match the applicable rules
				for (; it_pseudo_class_properties != it_pseudo_class_properties_end; ++it_pseudo_class_properties)
				{
					const PseudoClassPropertyList& pseudo_list = it_pseudo_class_properties->second;
					for (; i_pseudo_class < pseudo_list.size(); ++i_pseudo_class)
					{
						if (IsPseudoClassRuleApplicable(pseudo_list[i_pseudo_class].first, *pseudo_classes))
						{
							return;
						}
					}
					i_pseudo_class = 0;
				}
			}
		}
	};


	/// Returns an iterator to the first property matching the active set of pseudo_classes.
	/// Note: Modifying the element definition or pseudo classes invalidates the iterators.
	/// Note: The lifetime of pseudo_classes must extend beyond the iterators.
	Iterator begin(const StringList& pseudo_classes) const {
		return Iterator(pseudo_classes, properties.GetProperties().begin(), pseudo_class_properties.begin(), properties.GetProperties().end(), pseudo_class_properties.end());
	}
	/// Returns an iterator to the property following the last property matching the active set of pseudo_classes.
	Iterator end(const StringList& pseudo_classes) const {
		return Iterator(pseudo_classes, properties.GetProperties().end(), pseudo_class_properties.end(), properties.GetProperties().end(), pseudo_class_properties.end());
	}


protected:
	/// Destroys the definition.
	void OnReferenceDeactivate();

private:
	typedef std::pair< String, PropertyDictionary > PropertyGroup;
	typedef SmallUnorderedMap< String, PropertyGroup > PropertyGroupMap;

	typedef std::vector< std::pair< StringList, int > > PseudoClassFontEffectIndex;
	typedef SmallUnorderedMap< String, PseudoClassFontEffectIndex > FontEffectIndex;

	typedef SmallUnorderedMap< String, PseudoClassVolatility > PseudoClassVolatilityMap;

	// Finds all propery declarations for a group.
	void BuildPropertyGroup(PropertyGroupMap& groups, const String& group_type, const PropertyDictionary& element_properties, const PropertyGroupMap* default_properties = NULL);
	// Updates a property dictionary of all properties for a single group.
	int BuildPropertyGroupDictionary(PropertyDictionary& group_properties, const String& group_type, const String& group_name, const PropertyDictionary& element_properties);

	// Builds decorator definitions from the parsed properties and instances decorators as
	// appropriate.
	void InstanceDecorators(const PseudoClassPropertyMap& merged_pseudo_class_properties);
	// Attempts to instance a decorator.
	bool InstanceDecorator(const String& name, const String& type, const PropertyDictionary& properties, const StringList& pseudo_class = StringList());

	// Builds font effect definitions from the parsed properties and instances font effects as
	// appropriate.
	void InstanceFontEffects(const PseudoClassPropertyMap& merged_pseudo_class_properties);
	// Attempts to instance a font effect.
	bool InstanceFontEffect(const String& name, const String& type, const PropertyDictionary& properties, const StringList& pseudo_class = StringList());

	// Returns true if the pseudo-class requirement of a rule is met by a list of an element's pseudo-classes.
	static bool IsPseudoClassRuleApplicable(const StringList& rule_pseudo_classes, const PseudoClassList& element_pseudo_classes);

	// The attributes for the default state of the element, with no pseudo-classes.
	PropertyDictionary properties;
	// The overridden attributes for the element's pseudo-classes.
	PseudoClassPropertyDictionary pseudo_class_properties;

	// The instanced decorators for this element definition.
	DecoratorMap decorators;
	// The overridden decorators for the element's pseudo-classes.
	PseudoClassDecoratorMap pseudo_class_decorators;

	// The list of every decorator used by this element in every class.
	FontEffectList font_effects;
	// For each unique decorator name, this stores (in order of specificity) the name of the
	// pseudo-class that has a definition for it, and the index into the list of decorators.
	FontEffectIndex font_effect_index;

	// The list of volatile pseudo-classes in this definition, and how volatile they are.
	PseudoClassVolatilityMap pseudo_class_volatility;

	// True if this definition has the potential to change as sibling elements are added or removed.
	bool structurally_volatile;
};

}
}

#endif
