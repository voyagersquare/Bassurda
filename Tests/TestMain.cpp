// Offline test runner for Bassurda.
//
//   BassurdaTests                      run the DSP checks
//   BassurdaTests --render <dir>       also render a demo WAV per factory preset
//   BassurdaTests --snapshot <file>    also save a PNG of the editor (needs a display / xvfb)

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;

namespace
{
    constexpr double sr = 48000.0;
    constexpr int block = 256;
    int failures = 0;

    void check (bool ok, const String& what)
    {
        std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << what << std::endl;
        if (! ok) ++failures;
    }

    void setParam (BassurdaProcessor& p, const char* id, float value)
    {
        auto* param = p.apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    struct Event { int sample; int note; bool on; };

    // Renders `length` samples, firing the given note events.
    std::vector<float> render (BassurdaProcessor& p, int length, std::vector<Event> events)
    {
        std::vector<float> out ((size_t) length);
        AudioBuffer<float> buf (2, block);
        for (int pos = 0; pos < length; pos += block)
        {
            const int n = std::min (block, length - pos);
            AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, n);
            view.clear();
            MidiBuffer midi;
            for (auto& e : events)
                if (e.sample >= pos && e.sample < pos + n)
                    midi.addEvent (e.on ? MidiMessage::noteOn (1, e.note, (uint8) 110) : MidiMessage::noteOff (1, e.note),
                                   e.sample - pos);
            p.processBlock (view, midi);
            std::copy (view.getReadPointer (0), view.getReadPointer (0) + n, out.begin() + pos);
        }
        return out;
    }

    float peak (const std::vector<float>& v, int from = 0, int to = -1)
    {
        if (to < 0) to = (int) v.size();
        float m = 0.0f;
        for (int i = from; i < to; ++i) m = std::max (m, std::abs (v[(size_t) i]));
        return m;
    }

    bool allFinite (const std::vector<float>& v)
    {
        for (auto x : v) if (! std::isfinite (x)) return false;
        return true;
    }

    void testPresets()
    {
        std::cout << "Factory presets render cleanly" << std::endl;
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.setCurrentProgram (i);
            auto out = render (p, (int) sr, { { 100, 36, true }, { 12000, 43, true }, { 12100, 36, false },
                                              { 30000, 43, false } });
            const float pk = peak (out);
            check (allFinite (out) && pk > 0.05f && pk <= 1.0001f,
                   p.getProgramName (i) + "  (peak " + String (Decibels::gainToDecibels (pk), 1) + " dBFS)");
        }
    }

    void testPhaseRetrigger()
    {
        std::cout << "Phase retrigger: every hit is identical" << std::endl;
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);
        p.setCurrentProgram (1); // 808 Boom (with click + saturation)
        render (p, 48000, {});   // let parameter smoothing settle after the preset change

        const int hit = 48000;
        auto out = render (p, hit * 3, { { 1000, 36, true }, { 9000, 36, false },
                                         { 1000 + hit, 36, true }, { 9000 + hit, 36, false },
                                         { 1000 + 2 * hit, 36, true }, { 9000 + 2 * hit, 36, false } });
        float maxDiff = 0.0f;
        for (int i = 0; i < 4000; ++i)
        {
            maxDiff = std::max (maxDiff, std::abs (out[(size_t) (1000 + i)] - out[(size_t) (1000 + hit + i)]));
            maxDiff = std::max (maxDiff, std::abs (out[(size_t) (1000 + i)] - out[(size_t) (1000 + 2 * hit + i)]));
        }
        check (maxDiff < 1.0e-4f, "max difference between hits " + String (maxDiff, 7));

        // with phase reset off the hits differ (free-running oscillators)
        setParam (p, ParamID::phaseRetrig, 0.0f);
        setParam (p, ParamID::clickLevel, 0.0f);
        auto out2 = render (p, hit * 2, { { 1000, 36, true }, { 9000, 36, false },
                                          { 1000 + hit + 37, 36, true }, { 9000 + hit, 36, false } });
        float diff2 = 0.0f;
        for (int i = 0; i < 4000; ++i)
            diff2 = std::max (diff2, std::abs (out2[(size_t) (1000 + i)] - out2[(size_t) (1037 + hit + i)]));
        check (diff2 > 1.0e-3f, "free-running oscillators differ when phase reset is off");
    }

    void testGlide()
    {
        std::cout << "Glide (legato, linear, 100 ms)" << std::endl;
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);
        setParam (p, ParamID::glideMode, 1);
        setParam (p, ParamID::glideCurve, 0);
        setParam (p, ParamID::glideTime, 100.0f);

        render (p, 4800, { { 0, 36, true } });
        render (p, 2400, { { 0, 48, true } });                    // 50 ms into the glide
        const float mid = p.getEngine().uiCurrentNote.load();
        check (std::abs (mid - 42.0f) < 0.3f, "halfway through glide the pitch is " + String (mid, 2) + " (expected ~42)");
        render (p, 4800, {});
        const float end = p.getEngine().uiCurrentNote.load();
        check (std::abs (end - 48.0f) < 1.0e-3f, "glide lands exactly on the target note (" + String (end, 3) + ")");

        // non-legato note must jump immediately
        render (p, 2400, { { 0, 48, false }, { 0, 36, false } });
        render (p, 48000, {});
        render (p, 480, { { 0, 31, true } });
        const float jump = p.getEngine().uiCurrentNote.load();
        check (std::abs (jump - 31.0f) < 1.0e-3f, "detached note does not glide in legato mode (" + String (jump, 3) + ")");
    }

    void testCleanSubAndLatency()
    {
        std::cout << "Clean sub + latency compensation" << std::endl;
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);
        check (p.getLatencySamples() > 0, "oversampling latency reported to host: " + String (p.getLatencySamples()) + " samples");

        // sub only, clean: the output must be a pure sine (no harmonics),
        // even with heavy saturation
        setParam (p, ParamID::osc1On, 0.0f);
        setParam (p, ParamID::subOct, 0);
        setParam (p, ParamID::subClean, 1.0f);
        setParam (p, ParamID::satDrive, 1.0f);
        setParam (p, ParamID::aSus, 1.0f);
        setParam (p, ParamID::outClip, 0.0f);
        setParam (p, ParamID::monoCut, 0);
        auto out = render (p, 48000, { { 0, 45, true } });     // A2 = 110 Hz

        // measure the 2nd and 3rd harmonics relative to the fundamental
        auto bin = [&] (double f)
        {
            double re = 0, im = 0;
            for (int i = 24000; i < 48000; ++i)
            {
                re += out[(size_t) i] * std::cos (bdsp::twoPi * f * i / sr);
                im += out[(size_t) i] * std::sin (bdsp::twoPi * f * i / sr);
            }
            return std::sqrt (re * re + im * im);
        };
        const double h1 = bin (110.0), h2 = bin (220.0), h3 = bin (330.0);
        const double thdDb = Decibels::gainToDecibels (std::sqrt (h2 * h2 + h3 * h3) / h1);
        check (thdDb < -80.0, "clean sub harmonic distortion " + String (thdDb, 1) + " dB");

        // routed through the saturator it must gain harmonics
        setParam (p, ParamID::subClean, 0.0f);
        out = render (p, 48000, { { 0, 45, false }, { 100, 45, true } });
        const double g1 = bin (110.0), g2 = bin (220.0), g3 = bin (330.0);
        const double thd2 = Decibels::gainToDecibels (std::sqrt (g2 * g2 + g3 * g3) / g1);
        check (thd2 > -30.0, "saturated sub gains harmonics for small speakers: " + String (thd2, 1) + " dB");
    }

    void testPitchEnvelope()
    {
        std::cout << "Pitch envelope (808 drop)" << std::endl;
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);
        setParam (p, ParamID::osc1On, 0.0f);
        setParam (p, ParamID::subOct, 0);
        setParam (p, ParamID::pitchAmt, 12.0f);
        setParam (p, ParamID::pitchDecay, 50.0f);
        setParam (p, ParamID::satMix, 0.0f);
        auto out = render (p, 24000, { { 0, 45, true } });
        const int latency = p.getLatencySamples();

        // frequency from the distance between rising zero crossings
        auto freqAt = [&] (int from)
        {
            std::vector<double> xs;
            for (int i = from + 1; i < (int) out.size() && xs.size() < 2; ++i)
                if (out[(size_t) i - 1] < 0.0f && out[(size_t) i] >= 0.0f)
                    xs.push_back (i - 1 + out[(size_t) i - 1] / (out[(size_t) i - 1] - out[(size_t) i]));
            return xs.size() == 2 ? sr / (xs[1] - xs[0]) : 0.0;
        };
        const double early = freqAt (latency + 2);
        const double late  = freqAt (12000);
        check (early > late * 1.25 && std::abs (late - 110.0) < 1.0, "pitch starts high (" + String (early, 0) + " Hz) and drops to " + String (late, 0) + " Hz");
    }

    void renderDemos (const File& dir)
    {
        std::cout << "Rendering demo WAVs to " << dir.getFullPathName() << std::endl;
        dir.createDirectory();
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);

        // a short trap-style line: long 808 notes with overlapping slides
        const double bpm = 140.0;
        const int beat = (int) (sr * 60.0 / bpm);
        std::vector<Event> line {
            { 0, 36, true },                  { beat * 2 - 200, 36, false },
            { beat * 2, 36, true },           { beat * 3 - 200, 36, false },
            { beat * 3, 43, true },           { beat * 3 + beat / 2, 39, true },   // overlap -> slide
            { beat * 3 + beat / 2 + 50, 43, false },                              { beat * 5 - 200, 39, false },
            { beat * 5, 34, true },           { beat * 6, 41, true },              // slide up
            { beat * 6 + 50, 34, false },     { beat * 8 - 400, 41, false },
        };

        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.setCurrentProgram (i);
            p.getEngine().allNotesOff (true);
            auto out = render (p, beat * 8 + (int) sr / 2, line);

            AudioBuffer<float> buf (1, (int) out.size());
            buf.copyFrom (0, 0, out.data(), (int) out.size());
            auto file = dir.getChildFile (String (i + 1).paddedLeft ('0', 2) + "_" + p.getProgramName (i).replaceCharacter (' ', '_') + ".wav");
            file.deleteFile();
            WavAudioFormat wav;
            if (auto stream = std::unique_ptr<FileOutputStream> (file.createOutputStream()))
            {
                if (auto writer = std::unique_ptr<AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 1, 24, {}, 0)))
                {
                    stream.release();
                    writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
                }
            }
            std::cout << "  " << file.getFileName() << std::endl;
        }
    }

    void snapshot (const File& file)
    {
        BassurdaProcessor p;
        p.prepareToPlay (sr, block);
        p.setCurrentProgram (6); // G-Funk Moog: uses most sections
        render (p, 12000, { { 0, 36, true } });

        std::unique_ptr<AudioProcessorEditor> editor (p.createEditor());
        editor->setSize (MainView::designWidth, MainView::designHeight);
        // let the displays poll once
        for (int i = 0; i < 3; ++i)
        {
            render (p, 2048, {});
            MessageManager::getInstance()->runDispatchLoopUntil (40);
        }
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        file.deleteFile();
        FileOutputStream os (file);
        PNGImageFormat png;
        png.writeImageToStream (img, os);
        std::cout << "Saved snapshot " << file.getFullPathName() << std::endl;
    }
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;

    testPresets();
    testPhaseRetrigger();
    testGlide();
    testCleanSubAndLatency();
    testPitchEnvelope();

    for (int i = 1; i + 1 < argc; ++i)
    {
        const String arg (argv[i]);
        if (arg == "--render")   renderDemos (File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]));
        if (arg == "--snapshot") snapshot (File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]));
    }

    std::cout << (failures == 0 ? "\nAll tests passed." : "\nSome tests FAILED.") << std::endl;
    return failures == 0 ? 0 : 1;
}
