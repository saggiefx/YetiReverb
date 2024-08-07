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

namespace juce
{

//==============================================================================
/**
    Functions that allow sharing content between apps and devices.

    You can share text, images, files or an arbitrary data block.

    @tags{GUI}
*/
class JUCE_API ContentSharer
{
public:
    ContentSharer() = delete;

    /** A callback of this type is passed when starting a content sharing
        session.

        When the session ends, the function will receive a flag indicating
        whether the session was successful. In the case of failure, the
        errorText argument may hold a string describing the problem.
    */
    using Callback = std::function<void (bool success, const String& errorText)>;

    /** Shares the given files. Each URL should be either a full file path
        or it should point to a resource within the application bundle. For
        resources on iOS it should be something like "content/image.png" if you
        want to specify a file from application bundle located in "content"
        directory. On Android you should specify only a filename, without an
        extension.

        Upon completion you will receive a callback with a sharing result. Note:
        Sadly on Android the returned success flag may be wrong as there is no
        standard way the sharing targets report if the sharing operation
        succeeded. Also, the optional error message is always empty on Android.

        @param files        the files to share
        @param callback     a callback that will be called on the main thread
                            when the sharing session ends
        @param parent       the component that should be used to host the
                            sharing view
    */
    [[nodiscard]] static ScopedMessageBox shareFilesScoped (const Array<URL>& files,
                                                            Callback callback,
                                                            Component* parent = nullptr);

    /** Shares the given text.

        Upon completion you will receive a callback with a sharing result. Note:
        Sadly on Android the returned success flag may be wrong as there is no
        standard way the sharing targets report if the sharing operation
        succeeded. Also, the optional error message is always empty on Android.

        @param text         the text to share
        @param callback     a callback that will be called on the main thread
                            when the sharing session ends
        @param parent       the component that should be used to host the
                            sharing view
    */
    [[nodiscard]] static ScopedMessageBox shareTextScoped (const String& text,
                                                           Callback callback,
                                                           Component* parent = nullptr);

    /** A convenience function to share an image. This is useful when you have images
        loaded in memory. The images will be written to temporary files first, so if
        you have the images in question stored on disk already call shareFiles() instead.
        By default, images will be saved to PNG files, but you can supply a custom
        ImageFileFormat to override this. The custom file format will be owned and
        deleted by the sharer. e.g.

        @code
        Graphics g (myImage);
        g.setColour (Colours::green);
        g.fillEllipse (20, 20, 300, 200);
        Array<Image> images;
        images.add (myImage);
        ContentSharer::getInstance()->shareImages (images, myCallback);
        @endcode

        Upon completion you will receive a callback with a sharing result. Note:
        Sadly on Android the returned success flag may be wrong as there is no
        standard way the sharing targets report if the sharing operation
        succeeded. Also, the optional error message is always empty on Android.

        @param images       the images to share
        @param format       the file format to use when saving the images.
                            If no format is provided, a sensible default will
                            be used.
        @param callback     a callback that will be called on the main thread
                            when the sharing session ends
        @param parent       the component that should be used to host the
                            sharing view
    */
    [[nodiscard]] static ScopedMessageBox shareImagesScoped (const Array<Image>& images,
                                                             std::unique_ptr<ImageFileFormat> format,
                                                             Callback callback,
                                                             Component* parent = nullptr);

    /** A convenience function to share arbitrary data. The data will be written
        to a temporary file and then that file will be shared. If you have
        your data stored on disk already, call shareFiles() instead.

        Upon completion you will receive a callback with a sharing result. Note:
        Sadly on Android the returned success flag may be wrong as there is no
        standard way the sharing targets report if the sharing operation
        succeeded. Also, the optional error message is always empty on Android.

        @param mb           the data to share
        @param callback     a callback that will be called on the main thread
                            when the sharing session ends
        @param parent       the component that should be used to host the
                            sharing view
    */
    [[nodiscard]] static ScopedMessageBox shareDataScoped (const MemoryBlock& mb,
                                                           Callback callback,
                                                           Component* parent = nullptr);
};

} // namespace juce
