lication/x-vnd.Info-ZIP.UnZip";
#endif

extern "C" int main_stub( int argc, char *argv[] );

int main( int argc, char **argv )
{
    BApplication app( unzip_signature );

    int retval = main_stub( argc, argv );

    app.PostMessage( B_QUIT_REQUESTED );
    app.Run();

    return retval;
}
                                                                                                                                                                                                    