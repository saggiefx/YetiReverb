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

#include "../Application/jucer_Headers.h"
#include "jucer_PaintRoutine.h"
#include "jucer_JucerDocument.h"
#include "jucer_ObjectTypes.h"
#include "PaintElements/jucer_PaintElementUndoableAction.h"
#include "PaintElements/jucer_PaintElementPath.h"
#include "PaintElements/jucer_PaintElementImage.h"
#include "PaintElements/jucer_PaintElementGroup.h"
#include "UI/jucer_JucerDocumentEditor.h"
#include "../Application/jucer_Application.h"

//==============================================================================
PaintRoutine::PaintRoutine()
    : document (nullptr),
      backgroundColour (ProjucerApplication::getApp().lookAndFeel.findColour (backgroundColourId))
{
    clear();
}

PaintRoutine::~PaintRoutine()
{
    elements.clear(); // do this explicitly before the scalar destructor because these
                      // objects will be listeners on this object
}

//==============================================================================
void PaintRoutine::changed()
{
    if (document != nullptr)
        document->changed();
}

bool PaintRoutine::perform (UndoableAction* action, const String& actionName)
{
    if (document != nullptr)
        return document->getUndoManager().perform (action, actionName);

    std::unique_ptr<UndoableAction> deleter (action);
    action->perform();
    return false;
}

void PaintRoutine::setBackgroundColour (Colour newColour) noexcept
{
    backgroundColour = newColour;
    changed();
}

void PaintRoutine::clear()
{
    if (elements.size() > 0)
    {
        elements.clear();
        changed();
    }
}

//==============================================================================
class AddXmlElementAction   : public UndoableAction
{
public:
    AddXmlElementAction (PaintRoutine& routine_, XmlElement* xml_)
        : routine (routine_), xml (xml_)
    {
    }

    bool perform() override
    {
        showCorrectTab();
        PaintElement* newElement = routine.addElementFromXml (*xml, -1, false);
        jassert (newElement != nullptr);

        indexAdded = routine.indexOfElement (newElement);
        jassert (indexAdded >= 0);
        return indexAdded >= 0;
    }

    bool undo() override
    {
        showCorrectTab();
        routine.removeElement (routine.getElement (indexAdded), false);
        return true;
    }

    int getSizeInUnits() override    { return 10; }

    int indexAdded;

private:
    PaintRoutine& routine;
    std::unique_ptr<XmlElement> xml;

    void showCorrectTab() const
    {
        if (JucerDocumentEditor* const ed = JucerDocumentEditor::getActiveDocumentHolder())
            ed->showGraphics (&routine);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AddXmlElementAction)
};

PaintElement* PaintRoutine::addElementFromXml (const XmlElement& xml, const int index, const bool undoable)
{
    selectedPoints.deselectAll();

    if (undoable && document != nullptr)
    {
        AddXmlElementAction* action = new AddXmlElementAction (*this, new XmlElement (xml));
        document->getUndoManager().perform (action, "Add new element");

        return elements [action->indexAdded];
    }

    if (PaintElement* const newElement = ObjectTypes::createElementForXml (&xml, this))
    {
        elements.insert (index, newElement);
        changed();

        return newElement;
    }

    return nullptr;
}

PaintElement* PaintRoutine::addNewElement (PaintElement* e, const int index, const bool undoable)
{
    if (e != nullptr)
    {
        std::unique_ptr<PaintElement> deleter (e);
        std::unique_ptr<XmlElement> xml (e->createXml());

        e = addElementFromXml (*xml, index, undoable);
    }

    return e;
}

//==============================================================================
class DeleteElementAction   : public PaintElementUndoableAction <PaintElement>
{
public:
    explicit DeleteElementAction (PaintElement* const element)
        : PaintElementUndoableAction <PaintElement> (element),
          oldIndex (-1)
    {
        xml.reset (element->createXml());
        oldIndex = routine.indexOfElement (element);
    }

    bool perform() override
    {
        showCorrectTab();
        routine.removeElement (getElement(), false);
        return true;
    }

    bool undo() override
    {
        PaintElement* newElement = routine.addElementFromXml (*xml, oldIndex, false);
        showCorrectTab();
        return newElement != nullptr;
    }

    int getSizeInUnits() override    { return 10; }

private:
    std::unique_ptr<XmlElement> xml;
    int oldIndex;
};


void PaintRoutine::removeElement (PaintElement* element, const bool undoable)
{
    if (elements.contains (element))
    {
        if (undoable)
        {
            perform (new DeleteElementAction (element),
                     "Delete " + element->getTypeName());
        }
        else
        {
            selectedElements.deselect (element);
            selectedPoints.deselectAll();

            selectedPoints.changed (true);
            selectedElements.changed (true);

            elements.removeObject (element);
            changed();
        }
    }
}

//==============================================================================
class FrontOrBackElementAction  : public PaintElementUndoableAction <PaintElement>
{
public:
    FrontOrBackElementAction (PaintElement* const element, int newIndex_)
        : PaintElementUndoableAction <PaintElement> (element),
          newIndex (newIndex_)
    {
        oldIndex = routine.indexOfElement (element);
    }

    bool perform() override
    {
        showCorrectTab();

        PaintElement* e = routine.getElement (oldIndex);
        routine.moveElementZOrder (oldIndex, newIndex);
        newIndex = routine.indexOfElement (e);
        return true;
    }

    bool undo() override
    {
        showCorrectTab();
        routine.moveElementZOrder (newIndex, oldIndex);
        return true;
    }

private:
    int newIndex, oldIndex;
};

void PaintRoutine::moveElementZOrder (int oldIndex, int newIndex)
{
    jassert (elements [oldIndex] != nullptr);

    if (oldIndex != newIndex && elements [oldIndex] != nullptr)
    {
        elements.move (oldIndex, newIndex);
        changed();
    }
}

void PaintRoutine::elementToFront (PaintElement* element, const bool undoable)
{
    if (element != nullptr && elements.contains (element))
    {
        if (undoable)
            perform (new FrontOrBackElementAction (element, -1), "Move elements to front");
        else
            moveElementZOrder (elements.indexOf (element), -1);
    }
}

void PaintRoutine::elementToBack (PaintElement* element, const bool undoable)
{
    if (element != nullptr && elements.contains (element))
    {
        if (undoable)
            perform (new FrontOrBackElementAction (element, 0), "Move elements to back");
        else
            moveElementZOrder (elements.indexOf (element), 0);
    }
}

//==============================================================================
const char* const PaintRoutine::clipboardXmlTag = "PAINTELEMENTS";

void PaintRoutine::copySelectedToClipboard()
{
    if (selectedElements.getNumSelected() == 0)
        return;

    XmlElement clip (clipboardXmlTag);

    for (auto* pe : elements)
        if (selectedElements.isSelected (pe))
            clip.addChildElement (pe->createXml());

    SystemClipboard::copyTextToClipboard (clip.toString());
}

void PaintRoutine::paste()
{
    if (auto doc = parseXMLIfTagMatches (SystemClipboard::getTextFromClipboard(), clipboardXmlTag))
    {
        selectedElements.deselectAll();
        selectedPoints.deselectAll();

        for (auto* e : doc->getChildIterator())
            if (PaintElement* newElement = addElementFromXml (*e, -1, true))
                selectedElements.addToSelection (newElement);
    }
}

void PaintRoutine::deleteSelected()
{
    const SelectedItemSet<PaintElement*> temp1 (selectedElements);
    const SelectedItemSet<PathPoint*> temp2 (selectedPoints);

    if (temp2.getNumSelected() > 0)
    {
        selectedPoints.deselectAll();
        selectedPoints.changed (true); // synchronous message to get rid of any property components

        // if any points are selected, just delete them, and not the element, which may
        // also be selected..
        for (int i = temp2.getNumSelected(); --i >= 0;)
            temp2.getSelectedItem (i)->deleteFromPath();

        changed();
    }
    else if (temp1.getNumSelected() > 0)
    {
        selectedElements.deselectAll();
        selectedElements.changed (true);

        for (int i = temp1.getNumSelected(); --i >= 0;)
            removeElement (temp1.getSelectedItem (i), true);

        changed();
    }
}

void PaintRoutine::selectAll()
{
    if (selectedPoints.getNumSelected() > 0)
    {
        if (const PaintElementPath* path = selectedPoints.getSelectedItem (0)->owner)
            for (int i = 0; i < path->getNumPoints(); ++i)
                selectedPoints.addToSelection (path->getPoint (i));
    }
    else
    {
        for (int i = 0; i < elements.size(); ++i)
            selectedElements.addToSelection (elements.getUnchecked (i));
    }
}

void PaintRoutine::selectedToFront()
{
    const SelectedItemSet<PaintElement*> temp (selectedElements);

    for (int i = temp.getNumSelected(); --i >= 0;)
        elementToFront (temp.getSelectedItem (i), true);
}

void PaintRoutine::selectedToBack()
{
    const SelectedItemSet<PaintElement*> temp (selectedElements);

    for (int i = 0; i < temp.getNumSelected(); ++i)
        elementToBack (temp.getSelectedItem (i), true);
}

void PaintRoutine::alignTop()
{
    if (selectedElements.getNumSelected() > 1)
    {
        auto* main = selectedElements.getSelectedItem (0);
        auto yPos = main->getY();

        for (auto* other : selectedElements)
        {
            if (other != main)
                other->setPaintElementBoundsAndProperties (other, other->getBounds().withPosition (other->getX(),
                                                                                                   yPos), main, true);
        }
    }
}
void PaintRoutine::alignRight()
{
    if (selectedElements.getNumSelected() > 1)
    {
        auto* main = selectedElements.getSelectedItem (0);
        auto rightPos = main->getRight();

        for (auto* other : selectedElements)
        {
            if (other != main)
                other->setPaintElementBoundsAndProperties (other, other->getBounds().withPosition (rightPos - other->getWidth(),
                                                                                                   other->getY()), main, true);
        }
    }
}

void PaintRoutine::alignBottom()
{
    if (selectedElements.getNumSelected() > 1)
    {
        auto* main = selectedElements.getSelectedItem (0);
        auto bottomPos = main->getBottom();

        for (auto* other : selectedElements)
        {
            if (other != main)
                other->setPaintElementBoundsAndProperties (other, other->getBounds().withPosition (other->getX(),
                                                                                                   bottomPos - other->getHeight()), main, true);
        }
    }
}
void PaintRoutine::alignLeft()
{
    if (selectedElements.getNumSelected() > 1)
    {
        auto* main = selectedElements.getSelectedItem (0);
        auto xPos = main->getX();

        for (auto* other : selectedElements)
        {
            if (other != main)
                other->setPaintElementBoundsAndProperties (other, other->getBounds().withPosition (xPos,
                                                                                                   other->getY()), main, true);
        }
    }
}

void PaintRoutine::groupSelected()
{
    PaintElementGroup::groupSelected (this);
}

void PaintRoutine::ungroupSelected()
{
    const SelectedItemSet<PaintElement*> temp (selectedElements);

    for (int i = 0; i < temp.getNumSelected(); ++i)
        if (PaintElementGroup* const pg = dynamic_cast<PaintElementGroup*> (temp.getSelectedItem (i)))
            pg->ungroup (true);
}

void PaintRoutine::bringLostItemsBackOnScreen (const Rectangle<int>& parentArea)
{
    for (auto* c : elements)
    {
        auto r = c->getCurrentBounds (parentArea);

        if (! r.intersects (parentArea))
        {
            r.setPosition (parentArea.getCentreX(), parentArea.getCentreY());
            c->setCurrentBounds (r, parentArea, true);
        }
    }
}

void PaintRoutine::startDragging (const Rectangle<int>& parentArea)
{
    for (auto* c : elements)
    {
        auto r = c->getCurrentBounds (parentArea);

        c->getProperties().set ("xDragStart", r.getX());
        c->getProperties().set ("yDragStart", r.getY());
    }

    getDocument()->beginTransaction();
}

void PaintRoutine::dragSelectedComps (int dx, int dy, const Rectangle<int>& parentArea)
{
    getDocument()->getUndoManager().undoCurrentTransactionOnly();

    if (document != nullptr && selectedElements.getNumSelected() > 1)
    {
        dx = document->snapPosition (dx);
        dy = document->snapPosition (dy);
    }

    for (int i = 0; i < selectedElements.getNumSelected(); ++i)
    {
        PaintElement* const c = selectedElements.getSelectedItem (i);

        const int startX = c->getProperties() ["xDragStart"];
        const int startY = c->getProperties() ["yDragStart"];

        Rectangle<int> r (c->getCurrentBounds (parentArea));

        if (document != nullptr && selectedElements.getNumSelected() == 1)
        {
            r.setPosition (document->snapPosition (startX + dx),
                           document->snapPosition (startY + dy));
        }
        else
        {
            r.setPosition (startX + dx,
                           startY + dy);
        }

        c->setCurrentBounds (r, parentArea, true);
    }

    changed();
}

void PaintRoutine::endDragging()
{
    getDocument()->beginTransaction();
}

//==============================================================================
void PaintRoutine::fillWithBackground (Graphics& g, const bool drawOpaqueBackground)
{
    if ((! backgroundColour.isOpaque()) && drawOpaqueBackground)
    {
        g.fillCheckerBoard (Rectangle<float> ((float) g.getClipBounds().getRight(),
                                              (float) g.getClipBounds().getBottom()),
                            50.0f, 50.0f,
                            Colour (0xffdddddd).overlaidWith (backgroundColour),
                            Colour (0xffffffff).overlaidWith (backgroundColour));
    }
    else
    {
        g.fillAll (backgroundColour);
    }
}

void PaintRoutine::drawElements (Graphics& g, const Rectangle<int>& relativeTo)
{
    Component temp;
    temp.setBounds (relativeTo);

    for (auto* e : elements)
        e->draw (g, getDocument()->getComponentLayout(), relativeTo);
}

//==============================================================================
void PaintRoutine::dropImageAt (const File& f, int x, int y)
{
    std::unique_ptr<Drawable> d (Drawable::createFromImageFile (f));

    if (d != nullptr)
    {
        auto bounds = d->getDrawableBounds();
        d.reset();

        auto* newElement = addNewElement (ObjectTypes::createNewImageElement (this), -1, true);

        if (auto* pei = dynamic_cast<PaintElementImage*> (newElement))
        {
            String resourceName (getDocument()->getResources().findUniqueName (f.getFileName()));

            if (auto* existingResource = getDocument()->getResources().getResourceForFile (f))
            {
                resourceName = existingResource->name;
            }
            else
            {
                MemoryBlock data;
                f.loadFileAsData (data);

                getDocument()->getResources().add (resourceName, f.getFullPathName(), data);
            }

            pei->setResource (resourceName, true);

            const int imageW = (int) (bounds.getRight() + 0.999f);
            const int imageH = (int) (bounds.getBottom() + 0.999f);

            RelativePositionedRectangle pr;
            pr.rect.setX (x - imageW / 2);
            pr.rect.setY (y - imageH / 2);
            pr.rect.setWidth (imageW);
            pr.rect.setHeight (imageH);

            pei->setPosition (pr, true);

            getSelectedElements().selectOnly (pei);
        }
    }
}

//==============================================================================
const char* PaintRoutine::xmlTagName = "BACKGROUND";

XmlElement* PaintRoutine::createXml() const
{
    auto* xml = new XmlElement (xmlTagName);
    xml->setAttribute ("backgroundColour", backgroundColour.toString());

    for (auto* e : elements)
        xml->addChildElement (e->createXml());

    return xml;
}

bool PaintRoutine::loadFromXml (const XmlElement& xml)
{
    if (xml.hasTagName (xmlTagName))
    {
        backgroundColour = Colour::fromString (xml.getStringAttribute ("backgroundColour", Colours::white.toString()));

        clear();

        for (auto* e : xml.getChildIterator())
            if (auto* newElement = ObjectTypes::createElementForXml (e, this))
                elements.add (newElement);

        return true;
    }

    return false;
}

void PaintRoutine::fillInGeneratedCode (GeneratedCode& code, String& paintMethodCode) const
{
    if (! backgroundColour.isTransparent())
        paintMethodCode << "g.fillAll (" << CodeHelpers::colourToCode (backgroundColour) << ");\n\n";

    for (auto* e : elements)
        e->fillInGeneratedCode (code, paintMethodCode);
}

void PaintRoutine::applyCustomPaintSnippets (StringArray& snippets)
{
    for (auto* e : elements)
        e->applyCustomPaintSnippets (snippets);
}
