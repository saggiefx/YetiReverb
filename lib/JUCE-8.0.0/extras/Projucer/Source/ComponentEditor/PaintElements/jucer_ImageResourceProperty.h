/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "../../ProjectSaving/jucer_ResourceFile.h"

//==============================================================================
/**
    A property that lets you pick a resource to use as an image, or create a
    new one with a file selector.
*/
template <class ElementType>
class ImageResourceProperty    : public ChoicePropertyComponent,
                                 private ChangeListener
{
public:
    ImageResourceProperty (JucerDocument& doc,
                           ElementType* const e,
                           const String& name,
                           const bool allowChoiceOfNoResource_)
        : ChoicePropertyComponent (name),
          element (e), document (doc),
          allowChoiceOfNoResource (allowChoiceOfNoResource_)
    {
        refreshChoices();
        doc.addChangeListener (this);
    }

    ImageResourceProperty (ElementType* const e, const String& name,
                           const bool allowChoiceOfNoResource_ = false)
        : ChoicePropertyComponent (name),
          element (e), document (*e->getDocument()),
          allowChoiceOfNoResource (allowChoiceOfNoResource_)
    {
        refreshChoices();
        document.addChangeListener (this);
    }

    ~ImageResourceProperty() override
    {
        document.removeChangeListener (this);
    }

    //==============================================================================
    virtual void setResource (const String& newName) = 0;

    virtual String getResource() const = 0;

    //==============================================================================
    void setIndex (int newIndex) override
    {
        if (newIndex == 0)
        {
            document.getResources()
                .browseForResource ("Select an image file to add as a resource",
                                    "*.jpg;*.jpeg;*.png;*.gif;*.svg",
                                    File(),
                                    String(),
                                    [this] (String resource)
                                    {
                                        if (resource.isNotEmpty())
                                            setResource (resource);
                                    });
        }
        else
        {
            if (choices[newIndex] == getNoneText() && allowChoiceOfNoResource)
                setResource (String());
            else
                setResource (choices [newIndex]);
        }
    }

    int getIndex() const override
    {
        if (getResource().isEmpty())
            return -1;

        return choices.indexOf (getResource());
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        refresh();
    }

    void refreshChoices()
    {
        choices.clear();

        choices.add ("-- create a new image resource -- ");
        choices.add (String());

        if (allowChoiceOfNoResource)
            choices.add (getNoneText());

        choices.addArray (document.getResources().getResourceNames());

        const SourceCodeDocument& cpp = document.getCppDocument();

        if (Project* project = cpp.getProject())
        {
            JucerResourceFile resourceFile (*project);

            for (int i = 0; i < resourceFile.getNumFiles(); ++i)
            {
                const File& file = resourceFile.getFile (i);

                if (ImageFileFormat::findImageFormatForFileExtension (file))
                    choices.add (resourceFile.getClassName() + "::" + resourceFile.getDataVariableFor (file));
            }
        }
    }

    const char* getNoneText() noexcept      { return "<< none >>"; }

protected:
    mutable Component::SafePointer<ElementType> element;
    JucerDocument& document;
    const bool allowChoiceOfNoResource;
};
