// Stub gbversion.h for the gdbparse library build.
// The upstream build generates this from gbversion.h.in via CMake; we provide
// a minimal hand-written version sufficient for the files we compile.

#ifndef GBVERSION_H_INCLUDED_
#define GBVERSION_H_INCLUDED_

#define VERSION "1.9.0-gdbparse"
constexpr char kVersionSHA[]  = "";
constexpr char kVersionDate[] = "";
#define WEB_DOC_DIR "https://www.gpsbabel.org/htmldoc-1.9.0"

#endif // GBVERSION_H_INCLUDED_
