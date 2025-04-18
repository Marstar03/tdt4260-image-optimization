#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <omp.h>

#include "ppm.h"

// Image from:
// http://7-themes.com/6971875-funny-flowers-pictures.html

typedef struct {
     double red,green,blue;
} AccuratePixel;

typedef struct {
     int x, y;
     AccuratePixel *data;
} AccurateImage;

// Convert ppm to high precision format.
AccurateImage *convertToAccurateImage(PPMImage *image) {
	// Make a copy
	AccurateImage *imageAccurate;
	imageAccurate = (AccurateImage *)malloc(sizeof(AccurateImage));
	imageAccurate->data = (AccuratePixel*)malloc(image->x * image->y * sizeof(AccuratePixel));
	for(int i = 0; i < image->x * image->y; i++) {
		imageAccurate->data[i].red   = (double) image->data[i].red;
		imageAccurate->data[i].green = (double) image->data[i].green;
		imageAccurate->data[i].blue  = (double) image->data[i].blue;
	}
	imageAccurate->x = image->x;
	imageAccurate->y = image->y;
	
	return imageAccurate;
}

PPMImage * convertToPPPMImage(AccurateImage *imageIn) {
    PPMImage *imageOut;
    imageOut = (PPMImage *)malloc(sizeof(PPMImage));
    imageOut->data = (PPMPixel*)malloc(imageIn->x * imageIn->y * sizeof(PPMPixel));

    imageOut->x = imageIn->x;
    imageOut->y = imageIn->y;

    for(int i = 0; i < imageIn->x * imageIn->y; i++) {
        imageOut->data[i].red = imageIn->data[i].red;
        imageOut->data[i].green = imageIn->data[i].green;
        imageOut->data[i].blue = imageIn->data[i].blue;
    }
    return imageOut;
}

// blur one color channel
void blurIteration(AccurateImage *imageOut, AccurateImage *imageIn, int size) {
	
	// Iterate over each pixel
	for(int senterY = 0; senterY < imageIn->y; senterY++) {
	
		for(int senterX = 0; senterX < imageIn->x; senterX++) {


			// reduce unneccessary references
			int numberOfValuesInEachRow = imageIn->x; // R, G and B

			// For each pixel we compute the magic number

			// added sum for each colour
			double sumRed = 0;
			double sumGreen = 0;
			double sumBlue = 0;

			int countIncluded = 0;
			for(int y = -size; y <= size; y++) {
				int currentY = senterY + y;
				
				// Check if we are outside the bounds
				if(currentY < 0 || currentY >= imageIn->y)
					continue;

				for(int x = -size; x <= size; x++) {
					int currentX = senterX + x;

					if(currentX < 0 || currentX >= imageIn->x)
						continue;

					// Now we can begin
					//int numberOfValuesInEachRow = imageIn->x;
					int offsetOfThePixel = (numberOfValuesInEachRow * currentY + currentX);

					sumRed += imageIn->data[offsetOfThePixel].red;
					sumGreen += imageIn->data[offsetOfThePixel].green;
					sumBlue += imageIn->data[offsetOfThePixel].blue;
					
					// Keep track of how many values we have included
					countIncluded++;
				}
				
			}
			
			// Now we compute the final value
			double valueRed = sumRed / countIncluded;
			double valueGreen = sumGreen / countIncluded;
			double valueBlue = sumBlue / countIncluded;
			
			
			// Update the output image
			int offsetOfThePixel = (numberOfValuesInEachRow * senterY + senterX);
			imageOut->data[offsetOfThePixel].red = valueRed;
			imageOut->data[offsetOfThePixel].green = valueGreen;
			imageOut->data[offsetOfThePixel].blue = valueBlue;
		}

	}
	
}


// Perform the final step, and return it as ppm.
PPMImage * imageDifference(AccurateImage *imageInSmall, AccurateImage *imageInLarge) {
	PPMImage *imageOut;
	imageOut = (PPMImage *)malloc(sizeof(PPMImage));
	imageOut->data = (PPMPixel*)malloc(imageInSmall->x * imageInSmall->y * sizeof(PPMPixel));
	
	imageOut->x = imageInSmall->x;
	imageOut->y = imageInSmall->y;

	for(int i = 0; i < imageInSmall->x * imageInSmall->y; i++) {
		double value = (imageInLarge->data[i].red - imageInSmall->data[i].red);
		if(value > 255)
			imageOut->data[i].red = 255;
		else if (value < -1.0) {
			value = 257.0+value;
			if(value > 255)
				imageOut->data[i].red = 255;
			else
				imageOut->data[i].red = floor(value);
		} else if (value > -1.0 && value < 0.0) {
			imageOut->data[i].red = 0;
		} else {
			imageOut->data[i].red = floor(value);
		}

		value = (imageInLarge->data[i].green - imageInSmall->data[i].green);
		if(value > 255)
			imageOut->data[i].green = 255;
		else if (value < -1.0) {
			value = 257.0+value;
			if(value > 255)
				imageOut->data[i].green = 255;
			else
				imageOut->data[i].green = floor(value);
		} else if (value > -1.0 && value < 0.0) {
			imageOut->data[i].green = 0;
		} else {
			imageOut->data[i].green = floor(value);
		}

		value = (imageInLarge->data[i].blue - imageInSmall->data[i].blue);
		if(value > 255)
			imageOut->data[i].blue = 255;
		else if (value < -1.0) {
			value = 257.0+value;
			if(value > 255)
				imageOut->data[i].blue = 255;
			else
				imageOut->data[i].blue = floor(value);
		} else if (value > -1.0 && value < 0.0) {
			imageOut->data[i].blue = 0;
		} else {
			imageOut->data[i].blue = floor(value);
		}
	}
	return imageOut;
}


int main(int argc, char** argv) {
    // read image
    PPMImage *image;
    // select where to read the image from
    if(argc > 1) {
        // from file for debugging (with argument)
        image = readPPM("flower.ppm");
    } else {
        // from stdin for cmb
        image = readStreamPPM(stdin);
    }
	
	
	AccurateImage *imageAccurate1_tiny = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_tiny = convertToAccurateImage(image);
	
	// Process the tiny case:
	// removed redundant iterations since we process all colors at the same time
	blurIteration(imageAccurate2_tiny, imageAccurate1_tiny, 2);
	blurIteration(imageAccurate1_tiny, imageAccurate2_tiny, 2);
	blurIteration(imageAccurate2_tiny, imageAccurate1_tiny, 2);
	blurIteration(imageAccurate1_tiny, imageAccurate2_tiny, 2);
	blurIteration(imageAccurate2_tiny, imageAccurate1_tiny, 2);
	
	
	AccurateImage *imageAccurate1_small = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_small = convertToAccurateImage(image);
	
	// Process the small case:
	blurIteration(imageAccurate2_small, imageAccurate1_small, 3);
	blurIteration(imageAccurate1_small, imageAccurate2_small, 3);
	blurIteration(imageAccurate2_small, imageAccurate1_small, 3);
	blurIteration(imageAccurate1_small, imageAccurate2_small, 3);
	blurIteration(imageAccurate2_small, imageAccurate1_small, 3);

    // an intermediate step can be saved for debugging like this
//    writePPM("imageAccurate2_tiny.ppm", convertToPPPMImage(imageAccurate2_tiny));
	
	AccurateImage *imageAccurate1_medium = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_medium = convertToAccurateImage(image);
	
	// Process the medium case:
	blurIteration(imageAccurate2_medium, imageAccurate1_medium, 5);
	blurIteration(imageAccurate1_medium, imageAccurate2_medium, 5);
	blurIteration(imageAccurate2_medium, imageAccurate1_medium, 5);
	blurIteration(imageAccurate1_medium, imageAccurate2_medium, 5);
	blurIteration(imageAccurate2_medium, imageAccurate1_medium, 5);
	
	AccurateImage *imageAccurate1_large = convertToAccurateImage(image);
	AccurateImage *imageAccurate2_large = convertToAccurateImage(image);
	
	// Do each color channel
	blurIteration(imageAccurate2_large, imageAccurate1_large, 8);
	blurIteration(imageAccurate1_large, imageAccurate2_large, 8);
	blurIteration(imageAccurate2_large, imageAccurate1_large, 8);
	blurIteration(imageAccurate1_large, imageAccurate2_large, 8);
	blurIteration(imageAccurate2_large, imageAccurate1_large, 8);

	// calculate difference
	PPMImage *final_tiny = imageDifference(imageAccurate2_tiny, imageAccurate2_small);
    PPMImage *final_small = imageDifference(imageAccurate2_small, imageAccurate2_medium);
    PPMImage *final_medium = imageDifference(imageAccurate2_medium, imageAccurate2_large);
	// Save the images.
    if(argc > 1) {
        writePPM("flower_tiny.ppm", final_tiny);
        writePPM("flower_small.ppm", final_small);
        writePPM("flower_medium.ppm", final_medium);
    } else {
        writeStreamPPM(stdout, final_tiny);
        writeStreamPPM(stdout, final_small);
        writeStreamPPM(stdout, final_medium);
    }
	
}

