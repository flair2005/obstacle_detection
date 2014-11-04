/*
 * VideoSource.cc
 *
 *  Created on: Nov 2, 2014
 *      Author: john
 */
#include "VideoSource.h"

VideoSource::VideoSource() {
	mirSize = CVD::ImageRef();
}

void VideoSource::GetAndFillFrameBWandRGB(CVD::Image<CVD::byte> &imBW, CVD::Image<CVD::Rgb<CVD::byte> > &imRGB) {
}

CVD::ImageRef VideoSource::Size() {
	return mirSize;
}



