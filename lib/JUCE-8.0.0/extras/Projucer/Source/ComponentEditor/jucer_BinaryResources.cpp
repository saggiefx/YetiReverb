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
#include "jucer_JucerDocument.h"

//==============================================================================
BinaryResources& BinaryResources::operator= (const BinaryResources& other)
{
    for (auto* r : other.resources)
        add (r->name, r->originalFilename, r->data);

    return *this;
}

void BinaryResources::changed()
{
    if (document != nullptr)
    {
        document->changed();
        document->refreshAllPropertyComps();
    }
}

//==============================================================================
void BinaryResources::clear()
{
    if (resources.size() > 0)
    {
        resources.clear();
        changed();
    }
}

StringArray BinaryResources::getResourceNames() const
{
    StringArray s;

    for (auto* r : resources)
        s.add (r->name);

    return s;
}

BinaryResources::BinaryResource* BinaryResources::findResource (const String& name) const noexcept
{
    for (auto* r : resources)
        if (r->name == name)
            return r;

    return nullptr;
}

const BinaryResources::BinaryResource* BinaryResources::getResource (const String& name) const
{
    return findResource (name);
}

const BinaryResources::BinaryResource* BinaryResources::getResourceForFile (const File& file) const
{
    for (auto* r : resources)
        if (r->originalFilename == file.getFullPathName())
            return r;

    return nullptr;
}

bool BinaryResources::add (const String& name, const File& file)
{
    MemoryBlock mb;

    if (! file.loadFileAsData (mb))
        return false;

    add (name, file.getFullPathName(), mb);
    return true;
}

void BinaryResources::add (const String& name, const String& originalFileName, const MemoryBlock& data)
{
    auto* r = findResource (name);

    if (r == nullptr)
    {
        resources.add (r = new BinaryResource());
        r->name = name;
    }

    r->originalFilename = originalFileName;
    r->data = data;
    r->drawable.reset();

    changed();
}

bool BinaryResources::reload (const int index)
{
    return resources[index] != nullptr
            && add (resources [index]->name,
                    File (resources [index]->originalFilename));
}

void BinaryResources::browseForResource (const String& title,
                                         const String& wildcard,
                                         const File& fileToStartFrom,
                                         const String& resourceToReplace,
                                         std::function<void (String)> callback)
{
    chooser = std::make_unique<FileChooser> (title, fileToStartFrom, wildcard);
    auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (flags, [safeThis = WeakReference<BinaryResources> { this },
                                  resourceToReplace,
                                  callback] (const FileChooser& fc)
    {
        if (safeThis == nullptr)
        {
            NullCheckedInvocation::invoke (callback, String{});
            return;
        }

        const auto result = fc.getResult();

        auto resourceName = [safeThis, result, resourceToReplace]() -> String
        {
            if (result == File())
                return {};

            if (resourceToReplace.isEmpty())
                return safeThis->findUniqueName (result.getFileName());

            return resourceToReplace;
        }();

        if (resourceName.isNotEmpty())
        {
            if (! safeThis->add (resourceName, result))
            {
                auto options = MessageBoxOptions::makeOptionsOk (MessageBoxIconType::WarningIcon,
                                                                 TRANS ("Adding Resource"),
                                                                 TRANS ("Failed to load the file!"));
                safeThis->messageBox = AlertWindow::showScopedAsync (options, nullptr);

                resourceName.clear();
            }
        }

        NullCheckedInvocation::invoke (callback, resourceName);
    });
}

String BinaryResources::findUniqueName (const String& rootName) const
{
    auto nameRoot = build_tools::makeValidIdentifier (rootName, true, true, false);
    auto name = nameRoot;

    auto names = getResourceNames();
    int suffix = 1;

    while (names.contains (name))
        name = nameRoot + String (++suffix);

    return name;
}

void BinaryResources::remove (int i)
{
    if (resources[i] != nullptr)
    {
        resources.remove (i);
        changed();
    }
}

const Drawable* BinaryResources::getDrawable (const String& name) const
{
    if (auto* res = const_cast<BinaryResources::BinaryResource*> (getResource (name)))
    {
        if (res->drawable == nullptr && res->data.getSize() > 0)
            res->drawable = Drawable::createFromImageData (res->data.getData(),
                                                           res->data.getSize());

        return res->drawable.get();
    }

    return nullptr;
}

Image BinaryResources::getImageFromCache (const String& name) const
{
    if (auto* res = getResource (name))
        if (res->data.getSize() > 0)
            return ImageCache::getFromMemory (res->data.getData(), (int) res->data.getSize());

    return {};
}

void BinaryResources::loadFromCpp (const File& cppFileLocation, const String& cppFile)
{
    StringArray cpp;
    cpp.addLines (cppFile);

    clear();

    for (int i = 0; i < cpp.size(); ++i)
    {
        if (cpp[i].contains ("JUCER_RESOURCE:"))
        {
            StringArray tokens;
            tokens.addTokens (cpp[i].fromFirstOccurrenceOf (":", false, false), ",", "\"'");
            tokens.trim();
            tokens.removeEmptyStrings();

            auto resourceName = tokens[0];
            auto resourceSize = tokens[1].getIntValue();
            auto originalFileName = cppFileLocation.getSiblingFile (tokens[2].unquoted()).getFullPathName();

            jassert (resourceName.isNotEmpty() && resourceSize > 0);

            if (resourceName.isNotEmpty() && resourceSize > 0)
            {
                auto firstLine = i;

                while (i < cpp.size())
                    if (cpp [i++].contains ("}"))
                        break;

                auto dataString = cpp.joinIntoString (" ", firstLine, i - firstLine)
                                     .fromFirstOccurrenceOf ("{", false, false);

                MemoryOutputStream out;
                String::CharPointerType t (dataString.getCharPointer());
                int n = 0;

                while (! t.isEmpty())
                {
                    auto c = t.getAndAdvance();

                    if (c >= '0' && c <= '9')
                    {
                        n = n * 10 + (int) (c - '0');
                    }
                    else if (c == ',')
                    {
                        out.writeByte ((char) n);
                        n = 0;
                    }
                    else if (c == '}')
                    {
                        break;
                    }
                }

                jassert (resourceSize < (int) out.getDataSize() && resourceSize > (int) out.getDataSize() - 2);

                MemoryBlock mb (out.getData(), out.getDataSize());
                mb.setSize ((size_t) resourceSize);

                add (resourceName, originalFileName, mb);
            }
        }
    }
}

//==============================================================================
void BinaryResources::fillInGeneratedCode (GeneratedCode& code) const
{
    if (resources.size() > 0)
    {
        code.publicMemberDeclarations << "// Binary resources:\n";

        MemoryOutputStream defs;

        defs << "//==============================================================================\n";
        defs << "// Binary resources - be careful not to edit any of these sections!\n\n";

        for (auto* r : resources)
        {
            code.publicMemberDeclarations
                << "static const char* "
                << r->name
                << ";\nstatic const int "
                << r->name
                << "Size;\n";

            auto name = r->name;
            auto& mb = r->data;

            defs << "// JUCER_RESOURCE: " << name << ", " << (int) mb.getSize()
                << ", \""
                << File (r->originalFilename)
                    .getRelativePathFrom (code.document->getCppFile())
                    .replaceCharacter ('\\', '/')
                << "\"\n";

            String line1;
            line1 << "static const unsigned char resource_"
                  << code.className << "_" << name << "[] = { ";

            defs << line1;

            int charsOnLine = line1.length();

            for (size_t j = 0; j < mb.getSize(); ++j)
            {
                auto num = (int) (unsigned char) mb[j];
                defs << num << ',';

                charsOnLine += 2;
                if (num >= 10)   ++charsOnLine;
                if (num >= 100)  ++charsOnLine;

                if (charsOnLine >= 200)
                {
                    charsOnLine = 0;
                    defs << '\n';
                }
            }

            defs
              << "0,0};\n\n"
                 "const char* " << code.className << "::" << name
              << " = (const char*) resource_" << code.className << "_" << name
              << ";\nconst int "
              << code.className << "::" << name << "Size = "
              << (int) mb.getSize()
              << ";\n\n";
        }

        code.staticMemberDefinitions << defs.toString();
    }
}
