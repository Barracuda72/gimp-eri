//
// Configuration
///////////////////////////////////////////////////////////////////////////////

// If you have iconv() function. define HAVE_ICONV.
#ifndef _WIN32
#define HAVE_ICONV
#endif

// Native Language Support. (gettext, etc...)
/* #define ENABLE_NLS */
/* #define HAVE_LC_MESSAGES */

// According to the development document (parasite.txt), UTF-8 encoded comment 
// is prefered.  But current version of gimp (gimp-1.1.*) can't display such
// comment and regards comments are encoded in locale's native character
// encoding. If this symbol is defined, this plug-in will assume that
// comments are always encoded in UTF-8. Otherwise, this plug-in  guesses
// locale's native charactor encoding, and uses the charset.
// Note: If GDK_USE_UTF8_MBS is defined, this option is no meaning. ;-)
#undef COMMENT_UTF8
