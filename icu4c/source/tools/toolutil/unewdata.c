/*
*******************************************************************************
*
*   Copyright (C) 1999, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  unewdata.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999oct25
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "unicode/udata.h"
#include "unewdata.h"

struct UNewDataMemory {
    FileStream *file;
    uint16_t headerSize;
    uint8_t magic1, magic2;
};

U_CAPI UNewDataMemory * U_EXPORT2
udata_create(const char *dir, const char *type, const char *name,
             const UDataInfo *pInfo,
             const char *comment,
             UErrorCode *pErrorCode) {
    UNewDataMemory *pData;
    uint16_t headerSize, commentLength;
    char filename[512];
    uint8_t bytes[16];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(name==NULL || *name==0 || pInfo==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    /* allocate the data structure */
    pData=(UNewDataMemory *)uprv_malloc(sizeof(UNewDataMemory));
    if(pData==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    /* open the output file */
    if(dir==NULL) {
        dir=u_getDataDirectory();
    }
    if(dir!=NULL && *dir) {
        char *p = filename + strlen(dir) - 1;
        uprv_strcpy(filename, dir);
        if (*p != U_FILE_SEP_CHAR) {
            *++p = U_FILE_SEP_CHAR;
            *p = 0;
        }
    } else {
        filename[0]=0;
    }
    uprv_strcat(filename, name);
    if(type!=NULL && *type!=0) {
        uprv_strcat(filename, ".");
        uprv_strcat(filename, type);
    }
    pData->file=T_FileStream_open(filename, "wb");
    if(pData->file==NULL) {
        uprv_free(pData);
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

    /* write the header information */
    headerSize=pInfo->size+4;
    if(comment!=NULL && *comment!=0) {
        commentLength=uprv_strlen(comment)+1;
        headerSize+=commentLength;
    } else {
        commentLength=0;
    }

    /* write the size of the header, take padding into account */
    pData->headerSize=(headerSize+15)&~0xf;
    pData->magic1=0xda;
    pData->magic2=0x27;
    T_FileStream_write(pData->file, &pData->headerSize, 4);

    /* write the information data */
    T_FileStream_write(pData->file, pInfo, pInfo->size);

    /* write the comment */
    if(commentLength>0) {
        T_FileStream_write(pData->file, comment, commentLength);
    }

    /* write padding bytes to align the data section to 16 bytes */
    headerSize&=0xf;
    if(headerSize!=0) {
        headerSize=16-headerSize;
        uprv_memset(bytes, 0, headerSize);
        T_FileStream_write(pData->file, bytes, headerSize);
    }

    return pData;
}

U_CAPI uint32_t U_EXPORT2
udata_finish(UNewDataMemory *pData, UErrorCode *pErrorCode) {
    uint32_t fileLength=0;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(pData!=NULL) {
        if(pData->file!=NULL) {
            /* fflush(pData->file);*/
            fileLength=T_FileStream_size(pData->file);
            if(T_FileStream_error(pData->file)) {
                *pErrorCode=U_FILE_ACCESS_ERROR;
            } else {
                fileLength-=pData->headerSize;
            }
            T_FileStream_close(pData->file);
        }
        uprv_free(pData);
    }

    return fileLength;
}

U_CAPI void U_EXPORT2
udata_write8(UNewDataMemory *pData, uint8_t byte) {
    if(pData!=NULL && pData->file!=NULL) {
        T_FileStream_write(pData->file, &byte, 1);
    }
}

U_CAPI void U_EXPORT2
udata_write16(UNewDataMemory *pData, uint16_t word) {
    if(pData!=NULL && pData->file!=NULL) {
        T_FileStream_write(pData->file, &word, 2);
    }
}

U_CAPI void U_EXPORT2
udata_write32(UNewDataMemory *pData, uint32_t wyde) {
    if(pData!=NULL && pData->file!=NULL) {
        T_FileStream_write(pData->file, &wyde, 4);
    }
}

U_CAPI void U_EXPORT2
udata_writeBlock(UNewDataMemory *pData, const void *s, UTextOffset length) {
    if(pData!=NULL && pData->file!=NULL) {
        if(length>0) {
            T_FileStream_write(pData->file, s, length);
        }
    }
}

U_CAPI void U_EXPORT2
udata_writePadding(UNewDataMemory *pData, UTextOffset length) {
    static uint8_t padding[16]={
        0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa
    };
    if(pData!=NULL && pData->file!=NULL) {
        while(length>=16) {
            T_FileStream_write(pData->file, padding, 16);
            length-=16;
        }
        if(length>0) {
            T_FileStream_write(pData->file, padding, length);
        }
    }
}

U_CAPI void U_EXPORT2
udata_writeString(UNewDataMemory *pData, const char *s, UTextOffset length) {
    if(pData!=NULL && pData->file!=NULL) {
        if(length==-1) {
            length=uprv_strlen(s);
        }
        if(length>0) {
            T_FileStream_write(pData->file, s, length);
        }
    }
}

U_CAPI void U_EXPORT2
udata_writeUString(UNewDataMemory *pData, const UChar *s, UTextOffset length) {
    if(pData!=NULL && pData->file!=NULL) {
        if(length==-1) {
            length=u_strlen(s);
        }
        if(length>0) {
            T_FileStream_write(pData->file, s, length*sizeof(UChar));
        }
    }
}

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
