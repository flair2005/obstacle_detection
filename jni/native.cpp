#include <android_native_app_glue.h>

#include <errno.h>
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>

#include "ATANCamera.h"
#include <gvars3/instances.h>
#include <gvars3/gvars3.h>
#include <cvd/image.h>
#include <cvd/rgb.h>
#include <cvd/byte.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <queue>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#define  LOG_TAG    "OCV:libnative_activity"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


struct Engine
{
	android_app* app;
	cv::Ptr<cv::VideoCapture> capture;
	ATANCamera mpCamera;

};

static cv::Size calc_optimal_camera_resolution(const char* supported, int width, int height)
{
	int frame_width = 0;
	int frame_height = 0;

	size_t prev_idx = 0;
	size_t idx = 0;
	float min_diff = FLT_MAX;

	do
	{
		int tmp_width;
		int tmp_height;

		prev_idx = idx;
		while ((supported[idx] != '\0') && (supported[idx] != ','))
			idx++;

		sscanf(&supported[prev_idx], "%dx%d", &tmp_width, &tmp_height);

		int w_diff = width - tmp_width;
		int h_diff = height - tmp_height;
		if ((h_diff >= 0) && (w_diff >= 0))
		{
			if ((h_diff <= min_diff) && (tmp_height <= 720))
			{
				frame_width = tmp_width;
				frame_height = tmp_height;
				min_diff = h_diff;
			}
		}

		idx++; // to skip coma symbol

	} while(supported[idx-1] != '\0');

	return cv::Size(frame_width, frame_height);
}

static void engine_setup_tracking(Engine* engine) {
	// Initialize tracking
	CVD::ImageRef mirSize;
	CVD::Image<CVD::Rgb<CVD::byte> > mimFrameRGB;
	CVD::Image<CVD::byte> mimFrameBW;

    mimFrameBW.resize(mirSize);
    mimFrameRGB.resize(mirSize);
    // First, check if the camera is calibrated.
    // If not, we need to run the calibration widget.
    Vector<NUMTRACKERCAMPARAMETERS> vTest;

    vTest = GV3::get<Vector<NUMTRACKERCAMPARAMETERS> >("Camera.Parameters", ATANCamera::mvDefaultParams, HIDDEN);
    ATANCamera mpCamera = new ATANCamera("Camera");
    Vector<2> v2;
    if(v2==v2) ;
    if(vTest == ATANCamera::mvDefaultParams)
    {
    	__android_log_print(ANDROID_LOG_INFO,"System", "Camera.Parameters is not set, need to run the CameraCalibrator tool");
        //cout << endl;
        //cout << "! Camera.Parameters is not set, need to run the CameraCalibrator tool" << endl;
        //cout << "  and/or put the Camera.Parameters= line into the appropriate .cfg file." << endl;
        exit(1);
    }

	Map *mpMap = new Map;
	MapMaker *mpMapMaker = new MapMaker(*mpMap, *mpCamera);
	Tracker *mpTracker =new Tracker(mVideoSource.Size(), *mpCamera, *mpMap, *mpMapMaker);
}

static void engine_track_frame(Engine* engine, const CVD::Image<CVD::byte> &imFrameBW) {
	// Convert
	static gvar3<int> gvnDrawMap("DrawMap", 0, HIDDEN|SILENT);
	static gvar3<int> gvnDrawAR("DrawAR", 0, HIDDEN|SILENT);

	bool bDrawMap = mpMap->IsGood() && *gvnDrawMap;
	bool bDrawAR = mpMap->IsGood() && *gvnDrawAR;

	mpTracker->TrackFrame(imFrameBW, !bDrawAR && !bDrawMap);

}

static void engine_draw_frame(Engine* engine, const cv::Mat& frame)
{
	if (engine->app->window == NULL)
		return; // No window.

	ANativeWindow_Buffer buffer;
	if (ANativeWindow_lock(engine->app->window, &buffer, NULL) < 0)
	{
		LOGW("Unable to lock window buffer");
		return;
	}

	void* pixels = buffer.bits;

	int left_indent = (buffer.width-frame.cols)/2;
	int top_indent = (buffer.height-frame.rows)/2;

	for (int yy = top_indent; yy < std::min(frame.rows+top_indent, buffer.height); yy++)
	{
		unsigned char* line = (unsigned char*)pixels;
		memcpy(line+left_indent*4*sizeof(unsigned char), frame.ptr<unsigned char>(yy),
				std::min(frame.cols, buffer.width)*4*sizeof(unsigned char));
		// go to next line
		pixels = (int32_t*)pixels + buffer.stride;
	}
	ANativeWindow_unlockAndPost(engine->app->window);
}

static void engine_handle_cmd(android_app* app, int32_t cmd)
{
	Engine* engine = (Engine*)app->userData;
	switch (cmd)
	{
	case APP_CMD_INIT_WINDOW:
		if (app->window != NULL)
		{
			LOGI("APP_CMD_INIT_WINDOW");

			engine->capture = new cv::VideoCapture(0);

			union {double prop; const char* name;} u;
			u.prop = engine->capture->get(CV_CAP_PROP_SUPPORTED_PREVIEW_SIZES_STRING);

			int view_width = ANativeWindow_getWidth(app->window);
			int view_height = ANativeWindow_getHeight(app->window);

			cv::Size camera_resolution;
			if (u.name)
				camera_resolution = calc_optimal_camera_resolution(u.name, 640, 480);
			else {
				LOGE("Cannot get supported camera camera_resolutions");
				camera_resolution = cv::Size(ANativeWindow_getWidth(app->window),
						ANativeWindow_getHeight(app->window));
			}

			if ((camera_resolution.width != 0) && (camera_resolution.height != 0))
			{
				engine->capture->set(CV_CAP_PROP_FRAME_WIDTH, camera_resolution.width);
				engine->capture->set(CV_CAP_PROP_FRAME_HEIGHT, camera_resolution.height);
			}

			float scale = std::min((float)view_width/camera_resolution.width,
					(float)view_height/camera_resolution.height);

			if (ANativeWindow_setBuffersGeometry(app->window, (int)(view_width/scale),
					int(view_height/scale), WINDOW_FORMAT_RGBA_8888) < 0)
			{
				LOGE("Cannot set pixel format!");
				return;
			}

			LOGI("Camera initialized at resoution %dx%d", camera_resolution.width, camera_resolution.height);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		LOGI("APP_CMD_TERM_WINDOW");

		engine->capture->release();
		break;
	}
}

void android_main(android_app* app) {

	Engine engine;

	// Make sure glue isn't stripped.
	app_dummy();

	memset(&engine, 0, sizeof(engine));
	app->userData = &engine;
	app->onAppCmd = engine_handle_cmd;
	engine.app = app;

	float fps = 0;
	cv::Mat frame_color;
	cv::Mat frame_bw;
	std::queue<int64> time_queue;

	// loop waiting for stuff to do.
	while (1)
	{
		// Read all pending events.
		int ident;
		int events;
		android_poll_source* source;

		// Process system events
		while ((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
			// Process this event.
			if (source != NULL) {
				source->process(app, source);
			}

			// Check if we are exiting.
			if (app->destroyRequested != 0) {
				LOGI("Engine thread destroy requested!");
				return;
			}
		}

		int64 then;
		int64 now = cv::getTickCount();
		time_queue.push(now);

		// Capture frame from camera and draw it
		if (!engine.capture.empty()) {
			if (engine.capture->grab()) {
				engine.capture->retrieve(frame_color, CV_CAP_ANDROID_COLOR_FRAME_RGBA);
				engine.capture->retrieve(frame_bw, CV_CAP_ANDROID_GREY_FRAME);
			}

			engine_track_frame(&engine, frame_bw);

			char buffer[256];
			sprintf(buffer, "Display performance: %dx%d @ %.3f", frame_color.cols, frame_color.rows, fps);
			cv::putText(frame_color, std::string(buffer), cv::Point(8,64),
					cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(0,255,0,255));
			engine_draw_frame(&engine, frame_color);
		}

		if (time_queue.size() >= 2)
			then = time_queue.front();
		else
			then = 0;

		if (time_queue.size() >= 25)
			time_queue.pop();

		fps = time_queue.size() * (float)cv::getTickFrequency() / (now-then);
	}
}
