/*
 * open_cv_helpers.cpp
 *
 *  Created on: Nov 3, 2014
 *      Author: john
 */

#include "open_cv_helpers.h"

void copyMatToCVD(cv::Mat &mat, CVD::Image<CVD::byte> &cvd) {
	//CVD::SubImage<CVD::byte> image(CVD::ImageRef(mat.cols, mat.rows));
	//for
	const CVD::BasicImage<CVD::byte> image(mat.ptr(0), CVD::ImageRef(mat.cols, mat.rows));
	cvd.copy_from(image);
}


