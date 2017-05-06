/*
    ERI(Entis Rasterized Image) loader for GIMP
    Copyright (C) 2000  SAKAI Masahiro

        SAKAI Masahiro                  <ZVM01052@nifty.ne.jp>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <string>
#include <stack>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <assert.h>
#include <gdk/gdk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
//#include <libgimp/gimpintl.h>
#include <libintl.h>
#include "eritypes.h"
#include "extrueri.h"

#define _(x) (x)

namespace std{}
using namespace std;

//
// Internationalization stuff
///////////////////////////////////////////////////////////////////////////////

static
void INIT_I18N()
{
#ifdef HAVE_LC_MESSAGES
    setlocale(LC_MESSAGES, "");
#endif
    bindtextdomain("gimp-libgimp", LOCALEDIR);
    bindtextdomain("gimp-std-plugins", LOCALEDIR);
    textdomain("gimp-std-plugins");
}

//
// Memory handler stuff
///////////////////////////////////////////////////////////////////////////////

PVOID eriAllocateMemory(DWORD dwBytes)
{
    return static_cast<PVOID>(g_malloc(dwBytes));
}

void eriFreeMemory(PVOID pMemory)
{
    g_free(static_cast<gpointer>(pMemory));
}

//
// Arithmetic stuff
///////////////////////////////////////////////////////////////////////////////
DWORD ChopMulDiv(DWORD x, DWORD y, DWORD z)
{
#ifdef _MSC_VER
    DWORD   dwResult ;
    __asm {
        mov     eax, x
        mul     y
        div     z
        mov     dwResult, eax
    }
    return  dwResult ;
#else
    return (guint64)x * (guint64)y / (guint64)z;
#endif
}

DWORD RaiseMulDiv(DWORD x, DWORD y, DWORD z)
{
#ifdef _MSC_VER
    DWORD   dwResult ;
    __asm {
        mov     eax, x
        mul     y
        div     z
        add     edx, 0FFFFFFFFH
        adc     eax, 0
        mov     dwResult, eax
    }
    return  dwResult ;
#else
    return ((guint64)x * (guint64)y + (guint64)z - 1) / (guint64)z;
#endif
}

//
// Decoder Object
//////////////////////////////////////////////////////////////////////////////

class MyERIDecoder: public ERIDecoder
{
private:
    void MyRestoreRGB24(PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight)
    {
        PBYTE   ptrDstLine = ptrDst;
        PINT    ptrBufLine = m_ptrBuffer;
        INT     nBytesPerPixel = (INT) m_nDstBytesPerPixel;
        LONG    nBlockSamples = (LONG) m_nBlockArea;

        while (nHeight--) {
            PINT ptrNextBuf = ptrBufLine;
            ptrDst = ptrDstLine;

            int i = nWidth;
            while (i--) {
                ptrDst[0] = (BYTE) ptrNextBuf[nBlockSamples * 2]; // R
                ptrDst[1] = (BYTE) ptrNextBuf[nBlockSamples];     // G
                ptrDst[2] = (BYTE) *ptrNextBuf;                   // B
                ptrNextBuf++;
                ptrDst += nBytesPerPixel;
            }
            ptrBufLine += m_nBlockSize;
            ptrDstLine += nLineBytes;
        }
    }

    void MyRestoreRGBA32(PBYTE ptrDst, LONG nLineBytes, int nWidth, int nHeight)
    {
        PBYTE   ptrDstLine = ptrDst;
        PINT    ptrBufLine = m_ptrBuffer;
        LONG    nBlockSamples = (LONG) m_nBlockArea;

        while (nHeight--) {
            PINT ptrNextBuf = ptrBufLine;
            ptrDst = ptrDstLine;
            int i = nWidth;

            while (i--) {
                ptrDst[0] = (BYTE) ptrNextBuf[nBlockSamples * 2]; // R
                ptrDst[1] = (BYTE) ptrNextBuf[nBlockSamples];     // G
                ptrDst[2] = (BYTE) *ptrNextBuf;                   // B
                ptrDst[3] = (BYTE) ptrNextBuf[nBlockSamples * 3]; // A
                ptrNextBuf ++;
                ptrDst += 4;
            }
            ptrBufLine += m_nBlockSize;
            ptrDstLine += nLineBytes;
        }
    }

public:
    virtual int OnDecodedBlock(LONG line, LONG column) {
        gimp_progress_update((gdouble)(line * m_nWidthBlocks + column) /
                             (gdouble)(m_nWidthBlocks * m_nHeightBlocks - 1));
        return 0;
    }

    virtual PFUNC_RESTORE GetRestoreFunc(DWORD fdwFormatType,
                                         DWORD dwBitsPerPixel)
    {
        PFUNC_RESTORE result;
        result = ERIDecoder::GetRestoreFunc(fdwFormatType, dwBitsPerPixel);

        switch (fdwFormatType) {
        case ERI_RGB_IMAGE:
            switch (dwBitsPerPixel) {
            case 24:
            case 32:
                result = reinterpret_cast<PFUNC_RESTORE>(&MyERIDecoder::MyRestoreRGB24);
                break;
            default:
                break;
            }
            break;
        case ERI_RGBA_IMAGE:
            result = reinterpret_cast<PFUNC_RESTORE>(&MyERIDecoder::MyRestoreRGBA32);
            break;
        default:
            break;
        }

        return result;
    }
};


//
// File Object
//////////////////////////////////////////////////////////////////////////////

struct EMC_REC_HEADER
{
    BYTE  rec_id[8];
    QWORD rec_bytes;
    DWORD rec_base;
};

class ERIFile {
private:
    // Don't implement copy construcer!
    ERIFile& operator= (const ERIFile& f);

protected:
    FILE* file;
    stack<EMC_REC_HEADER> records;

public:
    ERIFile() {
        file = NULL;
    }

    ~ERIFile(void) {
        Close();
    }

    int Open(const string& filename) {
        file = fopen(filename.c_str(), "rb");
        if (!file)
            return -1;

        struct  EMC_FILE_HEADER
        {
                BYTE  EMCID[8];
                DWORD FileID;
                DWORD Reserved;
                char  Format[0x30];
        };

        // Read EMC File header
        EMC_FILE_HEADER efh;
        if (Read(&efh, sizeof(efh)) < sizeof(efh))
            return -1;

        // Collate EMCID and File ID
        if ((GUINT32_FROM_LE(efh.FileID) != 0x3000100) ||
            memcmp(efh.EMCID, "Entis\x1A\0\0", 8))
        {
            return -1;
        }

        return  0;
    }

    void Close(void) {
        while (!records.empty())
            records.pop();

        if (file) {
            fclose(file);
            file = NULL;
        }
    }

    DWORD Read(void * ptrBuffer, DWORD dwBytes) {
        return fread(ptrBuffer, 1, dwBytes, file);
    }

    DWORD GetPointer(void) const {
        return ftell(file);
    }

    void Seek(DWORD dwPointer) {
        fseek(file, dwPointer, SEEK_SET);
    }

    int DescendRecord() {
        EMC_REC_HEADER RecHdr;
        if (Read(RecHdr.rec_id, sizeof(RecHdr.rec_id))
            < sizeof(RecHdr.rec_id))
        {
            return -1;
        }

        if (Read(&RecHdr.rec_bytes, sizeof(RecHdr.rec_bytes))
            < sizeof(RecHdr.rec_bytes))
        {
            return -1;
        }
        RecHdr.rec_bytes = GUINT64_FROM_LE(RecHdr.rec_bytes);

	RecHdr.rec_base = GetPointer();

        records.push(RecHdr);
        return  0;
    }

    int AscendRecord() {
        if (records.empty())
            return -1;
        Seek(records.top().rec_bytes + records.top().rec_base);
        records.pop();
        return  0;
    }

    const EMC_REC_HEADER* GetRecHeader() const {
        if (records.empty()) {
            return NULL;
        } else {
            return &records.top();
        }
    }
};


union ENTIS_PALETTE {
    DWORD    PaletteNumber;
    DWORD    PixelCode;
    REAL32   ZOrder32;

    struct {
        BYTE Blue;
        BYTE Green;
        BYTE Red;
        BYTE Reserved;
    } rgb;

    struct {
        BYTE Blue;
        BYTE Green;
        BYTE Red;
        BYTE Alpha;
    } rgba;

    struct {
        BYTE Y;
        BYTE U;
        BYTE V;
        BYTE Reserved;
    } yuv;

    struct {
        BYTE Brightness;
        BYTE Saturation;
        BYTE Hue;
        BYTE Reserved;
    } hsb;
};

typedef ENTIS_PALETTE* PENTIS_PALETTE;


//
// Input interface object
//////////////////////////////////////////////////////////////////////////////

class ERIInputContext: public RLHDecodeContext
{
private:
    // Don't implement copy construcer!
    ERIInputContext& operator= (const ERIInputContext& f);

public:
    ERIFile* file;

public:
    ERIInputContext(ERIFile* file, ULONG nBufferingSize)
        : RLHDecodeContext(nBufferingSize)
    {
        this->file = file;
    }

    ULONG ReadNextData(PBYTE ptrBuffer, ULONG nBytes) {
        return file->Read(ptrBuffer, nBytes);
    }
};


//
// Misc stuff
//////////////////////////////////////////////////////////////////////////////

static
bool ReadHeaderAndCheckParameter(ERIFile& erifile, ERI_FILE_HEADER& efh,
                                 ERI_INFO_HEADER& eih)
{
    if (erifile.DescendRecord())
        return false;

    const EMC_REC_HEADER* pRecHdr = erifile.GetRecHeader();
    if (memcmp(pRecHdr->rec_id, "FileHdr ", 8))
        return  false;

    if (erifile.Read(&efh, sizeof(efh)) < sizeof(efh))
        return  false;
    erifile.AscendRecord();

    // convert endian
    efh.dwVersion           = GUINT32_FROM_LE(efh.dwVersion);
    efh.dwContainedFlag     = GUINT32_FROM_LE(efh.dwContainedFlag);
    efh.dwKeyFrameCount     = GUINT32_FROM_LE(efh.dwKeyFrameCount);
    efh.dwAllFrameTime      = GUINT32_FROM_LE(efh.dwAllFrameTime);

    // check header
    if (efh.dwContainedFlag != 0x01
        && efh.dwContainedFlag != 0x31
        && efh.dwContainedFlag != 0x11
        )
        return false;
    if (efh.dwFrameCount == 0)
        return false;

    if (erifile.DescendRecord())
        return false;
    pRecHdr = erifile.GetRecHeader();
    if (memcmp(pRecHdr->rec_id, "ImageInf", 8))
        return false;
    if (erifile.Read(&eih, sizeof(eih)) < sizeof(eih))
        return false;
    erifile.AscendRecord();

    // convert endian
    eih.dwVersion            = GUINT32_FROM_LE(eih.dwVersion);
    eih.fdwTransformation    = GUINT32_FROM_LE(eih.fdwTransformation);
    eih.dwArchitecture       = GUINT32_FROM_LE(eih.dwArchitecture);
    eih.fdwFormatType        = GUINT32_FROM_LE(eih.fdwFormatType);
    eih.nImageWidth          = GINT32_FROM_LE(eih.nImageWidth);
    eih.nImageHeight         = GINT32_FROM_LE(eih.nImageHeight);
    eih.dwBitsPerPixel       = GUINT32_FROM_LE(eih.dwBitsPerPixel);
    eih.dwClippedPixel       = GUINT32_FROM_LE(eih.dwClippedPixel);
    eih.dwSamplingFlags      = GUINT32_FROM_LE(eih.dwSamplingFlags);
    eih.dwQuantumizedBits[0] = GINT32_FROM_LE(eih.dwQuantumizedBits[0]);
    eih.dwQuantumizedBits[1] = GINT32_FROM_LE(eih.dwQuantumizedBits[1]);
    eih.dwAllottedBits[0]    = GUINT32_FROM_LE(eih.dwAllottedBits[0]);
    eih.dwAllottedBits[1]    = GUINT32_FROM_LE(eih.dwAllottedBits[1]);
    eih.dwBlockingDegree     = GUINT32_FROM_LE(eih.dwBlockingDegree);
    eih.dwLappedBlock        = GUINT32_FROM_LE(eih.dwLappedBlock);
    eih.dwFrameTransform     = GUINT32_FROM_LE(eih.dwFrameTransform);
    eih.dwFrameDegree        = GUINT32_FROM_LE(eih.dwFrameDegree);

//    g_message("eih.dwArchitecture = %d\n"
//              "eih.fdwFormatType = %d\n"
//              "eih.dwBitsPerPixel = %d\n"
//              "eih.dwBlockingDegree = %d\n",
//              eih.dwArchitecture, eih.fdwFormatType,
//              eih.dwBitsPerPixel, eih.dwBlockingDegree);

    // check header
    if (eih.fdwTransformation != 0x03020000)
        return false;

    if (eih.dwArchitecture != 0xFFFFFFFF && eih.dwArchitecture != 32)
        return false;

    return true;
};

//
// Locale and charset stuff
//////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_ICONV

#include <iconv.h>

static
string guess_internal_codeset()
{
#if defined(GDK_USE_UTF8_MBS) || defined(COMMENT_UTF8)
    return "UTF-8";
#else
    const gchar* loc_name = gtk_set_locale();
    string codeset;

    const gchar* cp = loc_name;
    cp += strcspn(cp, "@.+,");
    if (*cp == '.') {
        const gchar *endp = ++cp;
        while ((*endp != '\0') && (*endp != '@'))
            endp++;
        if (endp != cp) {
            codeset.assign(cp, endp - cp);
            for (int i = 0; i < endp-cp; i++)
                codeset[i] = toupper(codeset[i]);
        }
    }

    if (codeset == "EUC") {
        const struct {
            char* key;
            char* codeset;
        } euc_db[] = {
            {"ja",    "EUC-JP"},
            {"ko",    "EUC-KR"},
            {"zh_TW", "EUC-TW"},
            {"zh_CN", "EUC-CN"},
        };

        for (int i = 0; i < sizeof(euc_db) / sizeof(euc_db[0]); i++) {
            if (!strncasecmp(loc_name, euc_db[i].key, strlen(euc_db[i].key))) {
                codeset = euc_db[i].codeset;
                break;
            }
        }
    }

    if (codeset == "")
        codeset = "US-ASCII";

    const struct {
        char* alias;
        char* real_name;
    } alias_db[] = {
        {"JIS",  "ISO-2022-JP"},
        {"JIS7", "ISO-2022-JP"},
        {"UJIS", "EUC-JP"},
        {"SJIS", "SHIFT_JIS"}
    };
    for (int i = 0; i < sizeof(alias_db) / sizeof(alias_db[0]); i++) {
        if (codeset == alias_db[i].alias) {
            codeset = alias_db[i].real_name;
            break;
        }
    }

    return codeset;
#endif
}

//
// these code are borrowed from mgedit-0.0.19.
//   MGEDIT, a simple text editor for X
//   Copyright (C) 1999 Akira Higuchi <a-higuti@math.sci.hokudai.ac.jp>
///////////////////////////////////////////////////////////////////////////////

#define MGEDIT_ICONV_EILSEQ 1
#define MGEDIT_ICONV_E2BIG  2
#define MGEDIT_ICONV_EINVAL 3

#define DEBUG_ICONV(x)

static void *
mgedit_iconv_open(const char *tocode, const char *fromcode)
{
	iconv_t cd, *retval;
	cd = iconv_open(tocode, fromcode);
	if (cd == (iconv_t)-1)
		return NULL;
	retval = g_new(iconv_t, 1);
	*retval = cd;
	DEBUG_ICONV(printf("mgedit_iconv_open from %s to %s\n",
			   fromcode, tocode));
	return retval;
}

static void
mgedit_iconv_close(void *cdp)
{
	gint r;
	if (cdp == NULL)
		return;
	r = iconv_close(*(iconv_t *)cdp);
	assert(r == 0);
	g_free(cdp);
}

static size_t
mgedit_iconv
(void *cdp, /*const*/ char **inbuf, size_t *inleft, char **outbuf,
 size_t *outleft, gint *error_code)
{
	size_t r;
	DEBUG_ICONV(printf("IN  from, fromlen, to, tolen = %p %d %p %d\n",
			   inbuf ? (*inbuf) : 0,
			   inleft ? (*inleft) : 0,
			   outbuf ? (*outbuf) : 0,
			   outleft ? (*outleft) : 0));
	r = iconv(*(iconv_t *)cdp, inbuf, inleft, outbuf, outleft);
	DEBUG_ICONV(printf("OUT from, fromlen, to, tolen = %p %d %p %d\n",
			   inbuf ? (*inbuf) : 0,
			   inleft ? (*inleft) : 0,
			   outbuf ? (*outbuf) : 0,
			   outleft ? (*outleft) : 0));
	if (r == (size_t)-1) {
		DEBUG_ICONV(perror("mgedit_iconv"));
		switch (errno) {
		case EILSEQ:
			*error_code = MGEDIT_ICONV_EILSEQ;
			break;
		case E2BIG:
			*error_code = MGEDIT_ICONV_E2BIG;
			break;
		case EINVAL:
			*error_code = MGEDIT_ICONV_EINVAL;
			break;
		default:
			perror("iconv");
			abort();
		}
	}
	return r;
}

static gint
apply_iconv
(void *cdp, /*const*/ gchar *buffer, gint len, gchar **buffer_r, gint *len_r,
 size_t *iconv_error_pos_r)
{
	gchar *to;
	/*const*/ gchar *from_p, *from;
	size_t from_len, to_len, to_alloc;
	gint error_code;

	from_p = from = buffer;
	from_len = len;
	to_alloc = 4096;
	to = g_new(gchar, to_alloc);
	to_len = 0;
	do {
		gchar *to_p;
		size_t s, from_left, to_left;

		if (to_alloc < to_len + 4096) {
			to_alloc += 4096;
			to = (gchar*)g_realloc(to, to_alloc);
		}
		from_left = MIN(from + from_len - from_p, 256);
		to_p = to + to_len;
		to_left = 4096;
		error_code = 0;
		s = mgedit_iconv(cdp, &from_p, &from_left,
				 &to_p, &to_left, &error_code);
		if ((error_code == 0) && (from + from_len == from_p)) {
			/* Write the reset sequence. We don't realloc
			 * 'to' here in order not to hit a bug(?) in
			 * glibc-2.1. This iconv() call may cause
			 * E2BIG in theory. */
			s = mgedit_iconv(cdp, NULL, NULL,
					 &to_p, &to_left, &error_code);
			to_len = to_p - to;
			break;
		}
		to_len = to_p - to;
	} while (error_code != MGEDIT_ICONV_EILSEQ &&
		 from_p < from + from_len);
	if (error_code)
		*iconv_error_pos_r = from_p - from;
	*len_r = to_len;
	*buffer_r = to;
	return error_code;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* HAVE_ICONV */


static
string convert_to_internal(/*const*/ void* data, guint len)
{
    string result;

#if defined(GDK_USE_UTF8_MBS) || defined(COMMENT_UTF8)
    if (static_cast<guchar*>(data)[0] == 0xFF
        && static_cast<guchar*>(data)[1] == 0xFE) {
        // little endian UCS-2
        // Unicode1.1 is used, so we don't need to worry about surrogate pairs.

        int count = (len - 2) / 2;
        const guint16* src = static_cast<guint16*>(data) + 1;
        char* tmp = new char[count * 6];
        char* p = tmp;

        for (int i = 0; i < count; i++)
            p += g_unichar_to_utf8(GUINT16_FROM_LE(src[i]), p);
        result.assign(tmp, p - tmp);

        delete[] tmp;
        return result;

    } else if (static_cast<guchar*>(data)[0] == 0xFE
               && static_cast<guchar*>(data)[1] == 0xFF) {
        // big endian UCS-2
        // Unicode1.1 is used, so we don't need to worry about surrogate pairs.

        int count = (len - 2) / 2;
        const guint16* src = static_cast<guint16*>(data) + 1;
        char* tmp = new char[count * 6];
        char* p = tmp;

        for (int i = 0; i < count; i++)
            p += g_unichar_to_utf8(GUINT16_FROM_BE(src[i]), p);
        result.assign(tmp, p - tmp);

        delete[] tmp;
        return result;

    } else {
#if defined(_WIN32) && !defined(HAVE_ICONV)
        // hack
        gchar* tmp = new gchar[len + 1];
        gchar* dest;
        memcpy(tmp, data, len);
        tmp[len] = '\0';
        dest = g_filename_to_utf8(tmp);
        result.assign(dest);
        g_free(dest);
        delete[] tmp;
        return result;
#endif // defined(_WIN32) && !defined(HAVE_ICONV)
    }
#endif // defined(GDK_USE_UTF8_MBS) || defined(COMMENT_UTF8)

#ifdef HAVE_ICONV
    do {
        iconv_t cd;
        gchar* dest = NULL;
        gint dest_len;
        size_t error_code;
        string internal_code;
        string external_code;

        if (static_cast<guchar*>(data)[0] == 0xFF
            && static_cast<guchar*>(data)[1] == 0xFE) {
            external_code = "UNICODELITTLE";
            //static_cast<guint16*>(data) += 1;
            data = (void*)(((guint16*)data) + 1);
        }
        else if (static_cast<guchar*>(data)[0] == 0xFE
                 && static_cast<guchar*>(data)[1] == 0xFF) {
            external_code = "UNICODEBIG";
            //static_cast<guint16*>(data) += 1;
            data = (void*)(((guint16*)data) + 1);
        } else {
            external_code = "SHIFT_JIS";
        }

        internal_code = guess_internal_codeset();

        cd = iconv_open(internal_code.c_str(), external_code.c_str());
        if (cd == (iconv_t)-1) {
            g_message("iconv_open failed (to_code = %s, from_code = %s)",
                      internal_code.c_str(), external_code.c_str());
            break;
        }
        apply_iconv(&cd, static_cast<gchar*>(data), len, &dest, &dest_len,
                    &error_code);
        iconv_close(cd);

        result.assign(dest, dest_len);
        g_free(dest);
        return result;
    } while(false);
#endif // HAVE_ICONV

    result.assign(static_cast<char*>(data), len);
    return result;
}


//
// GIMP Interface
//////////////////////////////////////////////////////////////////////////////

static void query(void);
static void run(const char* name,
                int nparams,
                const GimpParam* param, int* nreturn_vals, GimpParam** return_vals);
static gint32 load_image(const string& filename);


GimpPlugInInfo PLUG_IN_INFO = {
    NULL,                       /* init_proc */
    NULL,                       /* quit_proc */
    query,                      /* query_proc */
    run,                        /* run_proc */
};

MAIN();

static
void query()
{
    static GimpParamDef load_args[] = {
        {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
        {GIMP_PDB_STRING, "filename", "The name of the file to load"},
        {GIMP_PDB_STRING, "raw_filename", "The name entered"},
    };
    static GimpParamDef load_return_vals[] = {
        {GIMP_PDB_IMAGE, "image", "Output image"},
    };
    static int nload_args = sizeof(load_args) / sizeof(load_args[0]);
    static int nload_return_vals = sizeof(load_return_vals)
        / sizeof(load_return_vals[0]);

    gimp_install_procedure("file_eri_load",
                           "loads files of the ERI",
                           "loads files of the ERI(Entis Rasterized Image). ERI is full-color ross-less compressed image format, designed by L.Entis. For detail of ERI, see http://www.entis.gr.jp/eri/ .",
                           "SAKAI Masahiro",
                           "SAKAI Masahiro <ZVM01052@nifty.ne.jp>",
                           "2000",
                           "<Load>/ERI",
                           NULL,
                           GIMP_PLUGIN,
                           nload_args, nload_return_vals,
                           load_args, load_return_vals);
    
    gimp_register_magic_load_handler("file_eri_load", "eri", "",
                                     "0&,string,Entis\x1A,6&,byte,0,7&,byte,0,"
                                     "8&,byte,0,9&,byte,1,10&,byte,0,11&,byte,48");
}


static
void run(const char* name,
         int nparams, const GimpParam* param,
         int* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[2];
    gint32 imageID;

    *nreturn_vals = 1;
    *return_vals = values;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

    if (!strcmp(name, "file_eri_load")) {
        INIT_I18N();
        imageID = load_image(param[1].data.d_string);
        if (imageID == -1)
            values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        else {
            *nreturn_vals = 2;
            values[0].data.d_status = GIMP_PDB_SUCCESS;
            values[1].type = GIMP_PDB_IMAGE;
            values[1].data.d_image = imageID;
        }
    }
}


gint32 load_image(const string& filename)
{
    ERIFile erifile;
    gint32 imageID = -1;

    if (erifile.Open(filename))
        return -1;

    // Read file header record
    const EMC_REC_HEADER* pRecHdr;
    if (erifile.DescendRecord())
        return -1;
    pRecHdr = erifile.GetRecHeader();
    if (memcmp(pRecHdr->rec_id, "Header  ", 8))
        return -1;

    //
    // Read header and check whether supported or not
    ERI_FILE_HEADER efh;
    ERI_INFO_HEADER eih;
    if (!ReadHeaderAndCheckParameter(erifile, efh, eih))
        return -1;

    //
    // Read text infomation
#ifdef GIMP_HAVE_PARASITES
    string copyright;
    string description;

    while (!erifile.DescendRecord()) {
        guint len;
        PBYTE data;

        pRecHdr = erifile.GetRecHeader();

        if (!memcmp(pRecHdr->rec_id, "cpyright", 8)) {
            data = new BYTE[pRecHdr->rec_bytes];
            len  = pRecHdr->rec_bytes;
            erifile.Read(data, len);
            copyright = convert_to_internal(data, len);
            delete[] data;
        } else if (!memcmp(pRecHdr->rec_id, "descript", 8)) {
            data = new BYTE[pRecHdr->rec_bytes];
            len  = pRecHdr->rec_bytes;
            erifile.Read(data, len);
            description = convert_to_internal(data, len);
            delete[] data;
        }

        erifile.AscendRecord();
    }
#endif // GIMP_HAVE_PARASITES

    erifile.AscendRecord();

    //
    // Open "Stream" record to load image
    if (erifile.DescendRecord())
        return -1;
    //
    pRecHdr = erifile.GetRecHeader();
    if (memcmp(pRecHdr->rec_id, "Stream  ", 8))
        return -1;
    //
    if (erifile.DescendRecord())
        return -1;
    pRecHdr = erifile.GetRecHeader();

    int cmap_size = 0;
    guchar cmap[256 * 3];
    memset(cmap, 0, sizeof(cmap));
    //
    if (!memcmp(pRecHdr->rec_id, "Palette ", 8)) {
        cmap_size = pRecHdr->rec_bytes / sizeof(ENTIS_PALETTE);
        cmap_size = max(cmap_size, 256);
        for (int i = 0; i < cmap_size; i++) {
            ENTIS_PALETTE pal;
            if (erifile.Read(&pal, sizeof(pal)) != sizeof(pal))
                break;
            cmap[i * 3 + 0] = pal.rgb.Red;
            cmap[i * 3 + 1] = pal.rgb.Green;
            cmap[i * 3 + 2] = pal.rgb.Blue;
        }
        erifile.AscendRecord();
        if (erifile.DescendRecord())
            return -1;
        pRecHdr = erifile.GetRecHeader();
    }

    if (memcmp(pRecHdr->rec_id, "ImageFrm", 8))
        return -1;
    //
    // Prepare for decoding
    RASTER_IMAGE_INFO rii;
    rii.fdwFormatType = eih.fdwFormatType;
    rii.nImageWidth   = eih.nImageWidth;
    rii.nImageHeight  = abs(eih.nImageHeight);
    rii.dwBitsPerPixel = eih.dwBitsPerPixel;
    rii.BytesPerLine   = rii.nImageWidth * rii.dwBitsPerPixel / 8;
    rii.ptrImageArray  = new BYTE[rii.BytesPerLine * rii.nImageHeight];

    //
    // Setup progress indicater
    gchar* progress;
    if (strrchr(filename.c_str(), G_DIR_SEPARATOR) != NULL)
        progress = g_strdup_printf(_("Loading %s:"), strrchr(filename.c_str(), G_DIR_SEPARATOR) + 1);
    else
        progress = g_strdup_printf(_("Loading %s:"), filename.c_str());
    gimp_progress_init(progress);
    g_free(progress);

    //
    // Setup Input/Output interface
    ERIInputContext eic(&erifile, 0x10000);
    MyERIDecoder myed;
    if (myed.Initialize(eih))
        return -1;

    //
    // Decode
    if (myed.DecodeImage(rii, eic, false))
        return -1;
    //
    erifile.AscendRecord();
    erifile.AscendRecord();


    GimpImageType     layer_type;
    GimpImageBaseType image_type;

    switch (eih.fdwFormatType & ERI_TYPE_MASK) {
    case ERI_RGB_IMAGE:
        if (eih.fdwFormatType & ERI_WITH_PALETTE) {
            image_type = GIMP_INDEXED;
            if (eih.fdwFormatType & ERI_USE_CLIPPING)
                layer_type = GIMP_INDEXEDA_IMAGE;
            else
                layer_type = GIMP_INDEXED_IMAGE;
        } else {
            image_type = GIMP_RGB;
            layer_type = GIMP_RGB_IMAGE;
        }
        break;
    case ERI_RGBA_IMAGE:
        image_type = GIMP_RGB;
        layer_type = GIMP_RGBA_IMAGE;
        break;
    case ERI_GRAY_IMAGE:
        image_type = GIMP_GRAY;
        layer_type = GIMP_GRAY_IMAGE;
        break;
    default:
        return -1;
    }
    //
    if ((eih.fdwFormatType & ERI_TYPE_MASK) != ERI_RGB_IMAGE) {
        if (eih.fdwFormatType & ERI_WITH_PALETTE ||
            eih.fdwFormatType & ERI_USE_CLIPPING)
            return -1;
    }

    imageID = gimp_image_new(rii.nImageWidth, rii.nImageHeight, image_type);
    gimp_image_set_filename(imageID, const_cast<char*>(filename.c_str()));
    
    gint layerID;
    layerID = gimp_layer_new(imageID, _("Background"),
                             rii.nImageWidth, rii.nImageHeight, layer_type,
                             100, GIMP_NORMAL_MODE);
    //gimp_layer_set_visible(layerID, TRUE);
    //gimp_image_add_layer(imageID, layerID, 0);        
    gimp_image_insert_layer(imageID, layerID, -1, 0);        

    PBYTE image_data;
    if (rii.dwBitsPerPixel == 16) {
        PWORD src;
        image_data = new BYTE[rii.nImageWidth * rii.nImageHeight * 3];
        src = static_cast<PWORD>(static_cast<void*>(rii.ptrImageArray));

        for (int i = 0; i < rii.nImageWidth * rii.nImageHeight; i++) {
            BYTE r, g, b;
            r = (src[i] >> 10) & 0x1F;
            g = (src[i] >> 5) & 0x1F;
            b = src[i] & 0x1F;
            r = (r << 3) | (r >> 2);
            g = (g << 3) | (g >> 2);
            b = (b << 3) | (b >> 2);
            image_data[i * 3 + 0] = r;
            image_data[i * 3 + 1] = g;
            image_data[i * 3 + 2] = b;
        }

    } else if (eih.fdwFormatType & ERI_USE_CLIPPING) {
        PBYTE src = rii.ptrImageArray;
        image_data = new BYTE[rii.nImageWidth * rii.nImageHeight * 2];

        for (int i = 0; i < rii.nImageWidth * rii.nImageHeight; i++) {
            image_data[i * 2] = src[i];
            if (src[i] == eih.dwClippedPixel)
                image_data[i * 2 + 1] = 0;
            else
                image_data[i * 2 + 1] = 0xFF;
        }

    } else {
        image_data = rii.ptrImageArray;
    }

    GimpDrawable* drawable;
    GimpPixelRgn pixel_rgn;
    drawable = gimp_drawable_get(layerID);
    gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0,
                        rii.nImageWidth, rii.nImageHeight, TRUE, FALSE);
    gimp_pixel_rgn_set_rect(&pixel_rgn, image_data, 0, 0,
                            rii.nImageWidth, rii.nImageHeight);
    gimp_drawable_flush(drawable);
    // flip "bottom up" image
    if (image_type != GIMP_INDEXED) {
        if (eih.nImageHeight < 0)
            gimp_flip(drawable->drawable_id, GIMP_ORIENTATION_VERTICAL);
    } else {
        if (eih.nImageHeight > 0)
            gimp_flip(drawable->drawable_id, GIMP_ORIENTATION_VERTICAL);
    }
    gimp_drawable_detach(drawable);

    if (rii.ptrImageArray != image_data)
        delete[] image_data;
    delete[] rii.ptrImageArray;

    //
    // Attach text infomation as parasite
    //
#ifdef GIMP_HAVE_PARASITES
    if (description.length())
        gimp_image_attach_new_parasite(imageID, "gimp-comment",
                                       GIMP_PARASITE_PERSISTENT,
                                       description.length(),
                                       const_cast<char*>(description.c_str()));

    if (copyright.length())
        gimp_image_attach_new_parasite(imageID, "eri/copyright",
                                       GIMP_PARASITE_PERSISTENT,
                                       copyright.length(),
                                       const_cast<char*>(copyright.c_str()));
#endif // GIMP_HAVE_PARASITES

    if (cmap_size != 0)
        gimp_image_set_cmap(imageID, cmap, cmap_size);

    return imageID;
}

