// Copyright (C) zerofrog(@gmail.com), 2008-2009
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//Lesser GNU General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef SIFT_FAST_H
#define SIFT_FAST_H

#define SIFTFAST_VERSION_MAJOR 1
#define SIFTFAST_VERSION_MINOR 3
#define SIFTFAST_VERSION_PATCH 0
#define SIFTFAST_VERSION_COMBINED(major, minor, patch) (((major) << 16) | ((minor) << 8) | (patch))
#define SIFTFAST_VERSION SIFTFAST_VERSION_COMBINED(SIFTFAST_VERSION_MAJOR, SIFTFAST_VERSION_MINOR, SIFTFAST_VERSION_PATCH)

#define SIFTFAST_VERSION_GE(major1, minor1, patch1, major2, minor2, patch2) (SIFTFAST_VERSION_COMBINED(major1, minor1, patch1) >= SIFTFAST_VERSION_COMBINED(major2, minor2, patch2))
#define SIFTFAST_VERSION_MINIMUM(major, minor, patch) SIFTFAST_VERSION_GE(SIFTFAST_VERSION_MAJOR, SIFTFAST_VERSION_MINOR, SIFTFAST_VERSION_PATCH, major, minor, patch)

typedef struct ImageSt {
    int rows, cols;          // Dimensions of image.
    float *pixels;          // 2D array of image pixels.
    int stride;             // how many floats until the next row
                            // (used to add padding to make rows aligned to 16 bytes)
} *Image;

typedef struct KeypointSt {
    float row, col;             // Subpixel location of keypoint.
    float scale, ori;           // Scale and orientation (range [-PI,PI])
    float descrip[128];     // Vector of descriptor values
    struct KeypointSt *next;    // Pointer to next keypoint in list.
    // used for extracting descriptors, not part of the the keypoint's frame
    int imageindex; /// index of image keypoint came from
    float fpyramidscale; // scale of the pyramid
} *Keypoint;

typedef struct {
    int DoubleImSize;
    int Scales;
    float InitSigma;
    float PeakThresh; // default: 0.04/Scales
} SiftParameters;

#ifdef __cplusplus
extern "C" {
#endif

Keypoint GetKeypoints(Image porgimage);
Keypoint GetKeypointFrames(Image porgimage);
void GetKeypointDescriptors(Image porgimage, Keypoint frames);
Image CreateImage(int rows, int cols);
void DestroyAllImages();
void DestroyAllResources();
void FreeKeypoints(Keypoint keypt);
#ifdef __cplusplus
}
#endif

#endif
