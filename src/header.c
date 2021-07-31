#include "header.h"

int readImage(const char *path, ImageData *img)
{
    // Check inputs
    if (!path || !img)
        return FAIL;

    // Init variables
    int retval = FAIL;
    int jpegSubsamp = 0, width = 0, height = 0;
    uint64_t imgSize = 0;

    uint8_t *compImg = nullptr;
    uint8_t *decompImg = nullptr;

    FILE *fptr = nullptr;
    tjhandle jpegDecompressor = nullptr;

    // Find file
    if ((fptr = fopen(path, "rb")) == NULL)
        goto cleanup;
    if (fseek(fptr, 0, SEEK_END) < 0 || ((imgSize = ftell(fptr)) < 0) || fseek(fptr, 0, SEEK_SET) < 0)
        goto cleanup;
    if (imgSize == 0)
        goto cleanup;

    // Read file
    compImg = (uint8_t *)malloc(imgSize * sizeof(uint8_t));
    if (fread(compImg, imgSize, 1, fptr) < 1)
        goto cleanup;

    // Decompress
    jpegDecompressor = tjInitDecompress();
    if (!jpegDecompressor)
        goto cleanup;

    retval = tjDecompressHeader2(jpegDecompressor, compImg, imgSize, &width, &height, &jpegSubsamp);
    if (retval < 0)
        goto cleanup;
    decompImg = (uint8_t *)malloc(width * height * COLOR_COMPONENTS * sizeof(uint8_t));
    retval = tjDecompress2(jpegDecompressor, compImg, imgSize, decompImg, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);
    if (retval < 0)
        goto cleanup;

    // Set output
    img->width = width;
    img->height = height;
    img->size = imgSize;
    img->data = decompImg;
    decompImg = nullptr;

cleanup:

    tjDestroy(jpegDecompressor);
    fclose(fptr);

    free(compImg);
    free(decompImg);

    return retval;
}

int saveImage(const char *path, ImageData *img)
{
    // Check inputs
    if (!path || !img)
        return FAIL;

    // Init variables
    int retval = FAIL;
    uint64_t outSize = 0;

    uint8_t *compImg = nullptr;

    tjhandle jpegCompressor = nullptr;
    FILE *fptr = nullptr;

    // Compress
    jpegCompressor = tjInitCompress();
    if (!jpegCompressor)
        goto cleanup;

    retval = tjCompress2(jpegCompressor, img->data, img->width, 0, img->height, TJPF_RGB, &compImg, &outSize, TJSAMP_444, JPEG_QUALITY, TJFLAG_FASTDCT);
    if (retval < 0)
        goto cleanup;

    // Write file
    retval = FAIL; // To simplfy if checks
    if ((fptr = fopen(path, "wb")) == NULL)
        goto cleanup;
    if (fwrite(compImg, outSize, 1, fptr) < 1)
        goto cleanup;

    // Clean ImageData (since the main aim of the app is write data to file)
    retval = SUCCESS;
    free(img->data);
    img->data = nullptr;
    img->height = 0;
    img->width = 0;
    img->size = 0;

cleanup:

    fclose(fptr);
    tjDestroy(jpegCompressor);
    tjFree(compImg);

    return retval;
}

int dimBackground(ImageData *img, float k, ImageData *out)
{
    if (!img)
        return FAIL;
    if (!out)
        out = img;

    uint8_t *ptrIn = img->data;
    uint8_t *ptrOut = out->data;
    uint64_t len = img->width * img->height * COLOR_COMPONENTS;

#pragma omp simd
    for (uint64_t idx = 0; idx < len; ++idx)
        ptrOut[idx] = ptrIn[idx] * k;

    return 0;
}