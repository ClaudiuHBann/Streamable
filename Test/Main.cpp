#include "pch.h"

TEST_CASE("Streamable", "[Streamable]")
{
    SECTION("")
    {
    }
}

int main(int argc, char **argv)
{
    int returnCode{};
    {
        Session session;

#if _DEBUG
        ConfigData configData{.showSuccessfulTests = true,
                              .shouldDebugBreak = true,
                              .noThrow = true,
                              .showInvisibles = true,
                              .filenamesAsTags = true,

                              .verbosity = Verbosity::High,
                              .warnings =
                                  WarnAbout::What(WarnAbout::What::NoAssertions | WarnAbout::What::UnmatchedTestSpec),
                              .showDurations = ShowDurations::Always,
                              .defaultColourMode = ColourMode::PlatformDefault};

        session.useConfigData(configData);
#endif // _DEBUG

        returnCode = session.applyCommandLine(argc, argv);
        if (!returnCode)
        {
            returnCode = session.run();
        }
    }

    return returnCode;
}
