///
/// udpsrc -> application/x-rtp -> rtph264depay -> avdec_h264 -> videoconvert -> autovideosink
/// https://gstreamer.freedesktop.org/documentation/tutorials/basic/dynamic-pipelines.html?gi-language=c
///


#include <stdio.h>

/// Gstreamer
#include <gst/gst.h>
#include "gst/app/gstappsink.h"  /// libgstapp-1.0.so.0.1603.0
/// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>  /// color convert
#include <iostream>


#define PORT 8160
#define WIDTH 800
#define HEIGHT 600


int
main (int argc, char *argv[])
{
	const gchar *nano_str;
	guint major, minor, micro, nano;
	
	GstElement *pipeline;
	GstBus *bus;
	GstMessage *msg;
	///GMainLoop *loop = NULL;

	/// Initialize GStreamer
	gst_init (&argc, &argv);
	gst_version (&major, &minor, &micro, &nano);
	if (nano == 1)
		nano_str = "(CVS)";
	else if (nano == 2)
		nano_str = "(Prerelease)";
	else
		nano_str = "";
  
	printf ("Linked against GStreamer %d.%d.%d %s\n", major, minor, micro, nano_str);
	
	///loop = g_main_loop_new (NULL, FALSE);  // Need to check this went ok
					
	GstElement *source = NULL, 
		*rtpdepay = NULL,
		*decoder = NULL, 
		*vidconvert = NULL, 
		*sink = NULL,
		*appsink = NULL,
		*fakesink = NULL;
					
	/// Create gstreamer elements
	source = gst_element_factory_make ("udpsrc", "stream-source");
	g_object_set (source, "port", PORT, NULL);
	
	/// There's a more automated way to do this
	GstCaps *rtpCaps = gst_caps_new_simple("application/x-rtp",
											"media", G_TYPE_STRING, "video",
											"clock-rate", G_TYPE_INT, 90000,
											"encoding-name", G_TYPE_STRING, "H264",
											"packetization-mode", G_TYPE_STRING, "1", //??
											"profile-level-id", G_TYPE_STRING, "640028",
											"sprop-parameter-sets", G_TYPE_STRING, "J2QAKKwrQGQJvywDxImo\,KO4fLA\=\=",
											"payload", G_TYPE_INT, 96,
											"ssrc", G_TYPE_UINT, 2646632795,
											"timestamp-offset", G_TYPE_UINT, 3060627220,
											"seqnum-offset", G_TYPE_UINT, 10891,
											NULL);
	g_object_set (source, "caps", rtpCaps, NULL);
	
	
	rtpdepay = gst_element_factory_make ("rtph264depay", "rtpdepay");
	
	decoder = gst_element_factory_make ("avdec_h264", "avh264decoder");
	
	vidconvert = gst_element_factory_make ("videoconvert", "vidconvert");
	
	sink = gst_element_factory_make ("autovideosink", "display_sink");
	
	appsink = gst_element_factory_make ("appsink", "app_sink");
	
	fakesink = gst_element_factory_make ("fakesink", "fake_sink");
		g_object_set (fakesink, "dump", 1, NULL);


	/// Create the empty pipeline
	pipeline = gst_pipeline_new ("test-pipeline");
	
	/// Check created Elements
	if (!pipeline || !source || !fakesink || !rtpdepay || !decoder || !vidconvert || !sink || !appsink)
	{
		g_printerr ("Not all elements could be created.\n");
		return -1;
	}
	
	/// Build the pipeline - use 'sink' to see images
	gst_bin_add_many (GST_BIN (pipeline), source, rtpdepay, decoder, vidconvert, appsink, NULL);
	
	
	/// Caps filter to enforce YUV->RGB conversion
	GstCaps *vidConv_to_appSink = gst_caps_new_simple ("video/x-raw",
														"format", G_TYPE_STRING, "RGB",
														"width", G_TYPE_INT, WIDTH,
														"height", G_TYPE_INT, HEIGHT,
														NULL);
														
	/// link the elements together - link several times if want to to branch
	gst_element_link_many(source, rtpdepay, decoder, vidconvert);
	gst_element_link_filtered(vidconvert, appsink, vidConv_to_appSink);

	/// Start playing
	gst_element_set_state (pipeline, GST_STATE_PLAYING); //https://gstreamer.freedesktop.org/documentation/gstreamer/gstelement.html?gi-language=c#gst_element_set_state
	printf("About to run loop now\n");
	//g_main_loop_run (loop);


	/// variables for frame buffer extraction
	GstSample *sample;
	int nChannels = 3;
	unsigned char cBuf[WIDTH * HEIGHT * nChannels * sizeof(char)];
	GstBuffer* buffer;
	GstMapInfo map;
	///opencv
	cv::Mat img;
	cv::namedWindow( "Open CV", cv::WINDOW_AUTOSIZE );

	int count=1;
	while(1)
	{	
		/// Wait until error or EOS
		///~ bus = gst_element_get_bus (pipeline);
		///~ msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
		///~ GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
		///usleep(1000000/30);
		
		/// Will block until sample is ready. In our case "sample" is encoded picture.
		sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
		if(sample == NULL)
		{
			fprintf(stderr, "gst_app_sink_pull_sample returned null\n");
			return -1;
		}
		
		/// Actual compressed image is stored inside GstSample. YUV420 ?
		buffer = gst_sample_get_buffer (sample);
		gst_buffer_map (buffer, &map, GST_MAP_READ);

		/// Allocate appropriate buffer to store compressed image
		memmove(cBuf, map.data, map.size);

		/// OpenCV convert fropm yuv and display
		img = cv::Mat(cv::Size(WIDTH,HEIGHT), CV_8UC3, cBuf, cv::Mat::AUTO_STEP);
		cv::cvtColor(img, img, cv::COLOR_RGB2BGR );  /// opencv loves bgr
		cv::imshow( "Open CV", img );
		char c = (char)cv::waitKey(5);/// Allowing 25 milliseconds frame processing time and initiating break condition//
		if (c == 27){ /// If 'Esc' break the loop//
			break;
		}

		gst_buffer_unmap (buffer, &map);
		gst_sample_unref (sample);

		printf("frame %d\n", count);
		count +=1;
	}



	/// free resources
	if (msg != NULL)
		gst_message_unref (msg);
	gst_object_unref (bus);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);
	///g_main_loop_unref (loop);
	
	
	printf("apa\n");
	
	return 0;
}

