/**
 * @file   SelfOrganizingMap.cpp
 * @brief  Plain-C functions for self organizing map.
 * @date   Oct 23, 2014
 * @author Bernd Doser, HITS gGmbH
 */

#include "ImageProcessingLib/Image.h"
#include "ImageProcessingLib/ImageProcessing.h"
#include "SelfOrganizingMap.h"
#include <ctype.h>
#include <float.h>
#include <iostream>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

std::ostream& operator << (std::ostream& os, Layout layout)
{
	if (layout == QUADRATIC) os << "quadratic";
	else if (layout == HEXAGONAL) os << "hexagonal";
	else os << "undefined";
	return os;
}

std::ostream& operator << (std::ostream& os, SOMInitialization init)
{
	if (init == ZERO) os << "zero";
	else if (init == RANDOM) os << "random";
	else os << "undefined";
	return os;
}

std::ostream& operator << (std::ostream& os, Point p)
{
	return os << "(" << p.x << "," << p.y << ")";
}

void generateRotatedImages(float *rotatedImages, float *image, int numberOfRotations, int image_dim, int neuron_dim)
{
	int image_size = image_dim * image_dim;
	int neuron_size = neuron_dim * neuron_dim;
	float angleStepRadians;
	if (numberOfRotations) angleStepRadians = 2.0 * M_PI / numberOfRotations;

	// Copy original image on first position
	crop(image_dim, image_dim, neuron_dim, neuron_dim, image, rotatedImages);

	// Rotate unflipped image
    #pragma omp parallel for
	for (int i = 1; i < numberOfRotations; ++i)	{
		rotateAndCrop(image_dim, image_dim, neuron_dim, neuron_dim, image, rotatedImages + i*neuron_size, i*angleStepRadians);
	}

	// Flip image
	float *flippedImage = (float *)malloc(image_size * sizeof(float));
	flip(image_dim, image_dim, image, flippedImage);

	float *flippedRotatedImage = rotatedImages + numberOfRotations * neuron_size;
	crop(image_dim, image_dim, neuron_dim, neuron_dim, flippedImage, flippedRotatedImage);

	// Rotate flipped image
    #pragma omp parallel for
	for (int i = 1; i < numberOfRotations; ++i)	{
		rotateAndCrop(image_dim, image_dim, neuron_dim, neuron_dim, flippedImage, flippedRotatedImage + i*neuron_size, i*angleStepRadians);
	}

	free(flippedImage);
}

void generateEuclideanDistanceMatrix(float *euclideanDistanceMatrix, int *bestRotationMatrix, int som_dim, float* som,
	int image_dim, int numberOfRotations, float* image)
{
	int som_size = som_dim * som_dim;
	int image_size = image_dim * image_dim;

	float tmp;
	float* pdist = euclideanDistanceMatrix;
	int* prot = bestRotationMatrix;

    for (int i = 0; i < som_size; ++i) euclideanDistanceMatrix[i] = FLT_MAX;

    for (int i = 0; i < som_size; ++i, ++pdist, ++prot) {
        #pragma omp parallel for private(tmp)
        for (int j = 0; j < 2*numberOfRotations; ++j) {
    	    tmp = calculateEuclideanDistance(som + i*image_size, image + j*image_size, image_size);
            #pragma omp critical
    	    if (tmp < *pdist) {
    	    	*pdist = tmp;
                *prot = j;
    	    }
        }
    }
}

Point findBestMatchingNeuron(float *euclideanDistanceMatrix, int som_dim)
{
	int som_size = som_dim * som_dim;
    float minDistance = FLT_MAX;
    Point bestMatch;

    for (int i = 0; i < som_dim; ++i) {
        for (int j = 0; j < som_dim; ++j) {
			if (euclideanDistanceMatrix[i*som_dim+j] < minDistance) {
				minDistance = euclideanDistanceMatrix[i*som_dim+j];
				bestMatch.x = i;
				bestMatch.y = j;
			}
		}
    }

    return bestMatch;
}

void updateNeurons(int som_dim, float* som, int image_dim, float* image, Point const& bestMatch, int *bestRotationMatrix)
{
	float factor;
	int image_size = image_dim * image_dim;
	float *current_neuron = som;

    for (int i = 0; i < som_dim; ++i) {
        for (int j = 0; j < som_dim; ++j) {
        	factor = gaussian(distance(bestMatch,Point(i,j)), UPDATE_NEURONS_SIGMA) * UPDATE_NEURONS_DAMPING;
        	updateSingleNeuron(current_neuron, image + bestRotationMatrix[i*som_dim+j]*image_size, image_size, factor);
        	current_neuron += image_size;
    	}
    }
}

void updateSingleNeuron(float* neuron, float* image, int image_size, float factor)
{
    for (int i = 0; i < image_size; ++i) {
    	neuron[i] -= (neuron[i] - image[i]) * factor;
    }
}

void writeSOM(float* som, int som_dim, int image_dim, std::string const& filename)
{
    PINK::Image<float> image(som_dim*image_dim,som_dim*image_dim);
    float *pixel = image.getPointerOfFirstPixel();
    float *psom = som;

    for (int i = 0; i < som_dim; ++i) {
        for (int j = 0; j < som_dim; ++j) {
            for (int k = 0; k < image_dim; ++k) {
                for (int l = 0; l < image_dim; ++l) {
        	        pixel[i*image_dim*som_dim*image_dim + k*image_dim*som_dim + j*image_dim + l] = *psom++;
            	}
            }
    	}
    }

    image.writeBinary(filename);
}

void showSOM(float* som, int som_dim, int image_dim)
{
    PINK::Image<float> image(som_dim*image_dim,som_dim*image_dim);
    float *pixel = image.getPointerOfFirstPixel();
    float *psom = som;

    for (int i = 0; i < som_dim; ++i) {
        for (int j = 0; j < som_dim; ++j) {
            for (int k = 0; k < image_dim; ++k) {
                for (int l = 0; l < image_dim; ++l) {
        	        pixel[i*image_dim*som_dim*image_dim + k*image_dim*som_dim + j*image_dim + l] = *psom++;
            	}
            }
    	}
    }

    image.show();
}

void writeRotatedImages(float* images, int image_dim, int numberOfRotations, std::string const& filename)
{
	int heigth = 2 * numberOfRotations * image_dim;
	int width = image_dim;
	int image_size = image_dim * image_dim;
    float *image = (float *)malloc(heigth * width * sizeof(float));

    for (int i = 0; i < 2 * numberOfRotations; ++i) {
        for (int j = 0; j < image_size; ++j) image[j + i*image_size] = images[j + i*image_size];
    }

    writeImageToBinaryFile(image, heigth, width, filename);
    free(image);
}

void showRotatedImages(float* images, int image_dim, int numberOfRotations)
{
	int heigth = 2 * numberOfRotations * image_dim;
	int width = image_dim;
	int image_size = image_dim * image_dim;
    float *image = (float *)malloc(heigth * width * sizeof(float));

    for (int i = 0; i < 2 * numberOfRotations; ++i) {
        for (int j = 0; j < image_size; ++j) image[j + i*image_size] = images[j + i*image_size];
    }

    showImage(image, heigth, width);
    free(image);
}

void showRotatedImagesSingle(float* images, int image_dim, int numberOfRotations)
{
	int image_size = image_dim * image_dim;
    for (int i = 0; i < 2 * numberOfRotations; ++i) {
        showImage(images + i*image_size, image_dim, image_dim);
    }
}

float distance(Point pos1, Point pos2)
{
    return sqrt(pow(pos1.x - pos2.x, 2) + pow(pos1.y - pos2.y, 2));
}

void stringToUpper(char* s)
{
	for (char *ps = s; *ps != '\0'; ++ps) *ps = toupper(*ps);
}

// 2.0 / ( math.sqrt(3.0 * sigma) * math.pow(math.pi, 0.25)) * (1- x**2.0 / sigma**2.0) * math.exp(-x**2.0/(2.0 * sigma**2))
float mexicanHat(float x, float sigma)
{
	float x2 = x * x;
	float sigma2 = sigma * sigma;
    return 2.0 / (sqrt(3.0 * sigma) * pow(M_PI, 0.25)) * (1.0 - x2/sigma2) * exp(-x2 / (2.0 * sigma2));
}

// 1.0 / (sigma * math.sqrt(2.0 * math.pi)) * math.exp(-1.0/2.0 * (x / sigma)**2 );
float gaussian(float x, float sigma)
{
    return 1.0 / (sigma * sqrt(2.0 * M_PI)) * exp(-0.5 * pow((x/sigma),2));
}
