/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id: DarkHelp.hpp 2822 2019-09-13 20:11:41Z stephane $
 */

#pragma once

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <opencv2/opencv.hpp>


/** @file
 * DarkHelp is a C++ helper layer for accessing Darknet.  It was developed and tested with AlexeyAB's fork of the
 * popular Darknet project:  https://github.com/AlexeyAB/darknet
 *
 * @note The original darknet.h header file defines structures in the global namespace with names such as "image" and
 * "network" which are likely to cause problems in large existing projects.  For this reason, the DarkHelp class uses
 * a void* for the network and will only include darknet.h if explicitly told it can.
 *
 * Unless you are using darknet's "image" class directly in your application, it is probably best to NOT define this
 * macro, and not include darknet.h.  (The header is included by DarkHelp.cpp, so you definitely still need to have it,
 * but the scope of where it is needed is confined to that one .cpp file.)
 */
#ifdef DARKHELP_CAN_INCLUDE_DARKNET
#include <darknet.h>
#endif


/** Instantiate one of these objects by giving it the name of the .cfg and .weights file,
 * then call @ref predict() as often as necessary to determine what the images contain.
 * For example:
 * ~~~~
 * DarkHelp darkhelp("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
 *
 * const auto image_filenames = {"image_0.jpg", "image_1.jpg", "image_2.jpg"};
 *
 * for (const auto & filename : image_filenames)
 * {
 *     // these next two lines is where DarkHelp calls into Darknet to do all the hard work
 *     darkhelp.predict(filename);
 *     cv::Mat mat = darkhelp.annotate(); // annotates the most recent image seen by predict()
 *
 *     // use standard OpenCV calls to show the image results in a window
 *     cv::imshow("prediction", mat);
 *     cv::waitKey();
 * }
 * ~~~~
 *
 * Instead of calling @ref annotate(), you can get the detection results and iterate through them:
 *
 * ~~~~
 * DarkHelp darkhelp("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
 *
 * const auto results = darkhelp.predict("test_image_01.jpg");
 *
 * for (const auto & det : results)
 * {
 *     std::cout << det.name << " (" << 100.0 * det.best_probability << "% chance that this is class #" << det.best_class << ")" << std::endl;
 * }
 * ~~~~
 *
 * Instead of writing your own loop, you can also use the @p std::ostream @p operator<<() like this:
 *
 * ~~~~
 * const auto results = darkhelp.predict("test_image_01.jpg");
 * std::cout << results << std::endl;
 * ~~~~
 */
class DarkHelp
{
	public:

		/// Vector of text strings.  Typically used to store the class names.
		typedef std::vector<std::string> VStr;

		/// Vector of colours to use by @ref annotate().  @see @ref annotation_colours
		typedef std::vector<cv::Scalar> VColours;

		/** Map of a class ID to a probability that this object belongs to that class.
		 * The key is the zero-based index of the class, while the value is the probability
		 * that the object belongs to that class.
		 */
		typedef std::map<int, float> MClassProbabilities;

		/** Structure used to store interesting information on predictions.  A vector of these is created and returned to the
		 * caller every time @ref predict() is called.  The most recent predictions are also stored in @ref prediction_results.
		 */
		struct PredictionResult
		{
			/** OpenCV rectangle which describes where the object is located in the original image.
			 * @see @ref mid_x @see @ref mid_y @see @ref width @see @ref height
			 */
			cv::Rect rect;

			/** The original X coordinate returned by darknet.  This is the normalized mid-point, not the corner.
			 * You probably want to use @p rect.x instead of this value.  @see @ref rect
			 */
			float mid_x;

			/** The original Y coordinate returned by darknet.  This is the normalized mid-point, not the corner.
			 * You probably want to use @p rect.y instead of this value.  @see @ref rect
			 */
			float mid_y;

			/** The original width returned by darknet.  This value is normalized.
			 * You probably want to use @p rect.width instead of this value.  @see @ref rect
			 */
			float width;

			/** The original height returned by darknet.  This value is normalized.
			 * You probably want to use @p rect.height instead of this value.  @see @ref rect
			 */
			float height;

			/** This is only useful if you have multiple classes, and an object may be one of several possible classes.
			 *
			 * @note This will contain all @em non-zero class/probability pairs.
			 *
			 * For example, if your classes in your @p names file are defined like this:
			 * ~~~~{.txt}
			 * car
			 * person
			 * truck
			 * bus
			 * ~~~~
			 *
			 * Then an image of a truck may be 10.5% car, 0% person, 95.8% truck, and 60.3% bus.  Only the non-zero
			 * values are ever stored in this map, which for this example would be the following:
			 *
			 * @li 0 -> 0.105 // car
			 * @li 2 -> 0.958 // truck
			 * @li 3 -> 0.603 // bus
			 *
			 * (Note how @p person is not stored in the map, since the probability for that class is 0%.)
			 *
			 * In addition to @p %all_probabilities, the best results will @em also be duplicated in @ref best_class
			 * and @ref best_probability, which in this example would contain the values representing the truck:
			 *
			 * @li @ref best_class == 2
			 * @li @ref best_probability == 0.958
			 */
			MClassProbabilities all_probabilities;

			/** The class that obtained the highest probability.  For example, if an object is predicted to be 80% car
			 * or 60% truck, then the class id of the car would be stored in this variable.
			 * @see @ref best_probability
			 * @see @ref all_probabilities
			 */
			int best_class;

			/** The probability of the class that obtained the highest value.  For example, if an object is predicted to
			 * be 80% car or 60% truck, then the value of 0.80 would be stored in this variable.
			 * @see @ref best_class
			 * @see @ref all_probabilities
			 */
			float best_probability;

			/** A name to use for the object.  If an object has multiple probabilities, then the one with the highest
			 * probability will be listed first.  For example, a name could be @p "car 80%, truck 60%".  The @p name
			 * is used as a label when calling @ref annotate().  @see @ref names_include_percentage
			 */
			std::string name;
		};

		/** A vector of predictions for the image analyzed by @ref predict().  Each @ref PredictionResult entry in the
		 * vector represents a different object in the image.
		 * @see @ref PredictionResult
		 * @see @ref prediction_results
		 */
		typedef std::vector<PredictionResult> PredictionResults;

		/// Destructor.
		virtual ~DarkHelp();

		/// Constructor.
		DarkHelp(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "");

		/** Use the neural network to predict what is contained in this image.
		 * @param [in] image_filename The name of the image file to load from disk and analyze.  The member
		 * @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 * @see @ref PredictionResult
		 * @see @ref duration
		 */
		virtual PredictionResults predict(const std::string & image_filename, const float new_threshold = -1.0f);

		/** Use the neural network to predict what is contained in this image.
		 * @param [in] mat A OpenCV2 image which has already been loaded and which needs to be analyzed.  The member
		 * @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 * @see @ref PredictionResult
		 * @see @ref duration
		 */
		virtual PredictionResults predict(cv::Mat mat, const float new_threshold = -1.0f);

#ifdef DARKHELP_CAN_INCLUDE_DARKNET
		/** Use the neural network to predict what is contained in this image.
		 * @param [in] mat A Darknet-style image object which has already been loaded and which needs to be analyzed.
		 * The member @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 * @see @ref PredictionResult
		 * @see @ref duration
		 */
		virtual PredictionResults predict(image img, const float new_threshold = -1.0f);
#endif

		/** Takes the most recent @ref prediction_results, and applies them to the most recent @ref original_image.
		 * The output annotated image is stored in @ref annotated_image as well as returned to the caller.
		 *
		 * This is an example of what an annotated image looks like:
		 * @image html barcode_100_percent.png
		 *
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.
		 *
		 * Turning @em down the threshold in @ref annotate() wont bring back predictions that were excluded due to a
		 * higher threshold originally used with @ref predict().  Here is an example:
		 *
		 * ~~~~
		 * darkhelp.predict("image.jpg", 0.75);  // note the threshold is set to 75% for prediction
		 * darkhelp.annotate(0.25);              // note the threshold is now modified to be 25%
		 * ~~~~
		 *
		 * In the previous example, when annotate() is called with the lower threshold of 25%, the predictions had already
		 * been capped at 75%.  This means any prediction between >= 25% and < 75% were excluded from the prediction results.
		 * The only way to get those predictions is to re-run predict() with a value of 0.25.
		 *
		 * @see @ref annotation_colours
		 * @see @ref annotation_font_scale
		 * @see @ref annotation_font_thickness
		 * @see @ref annotation_include_duration
		 * @see @ref annotation_include_timestamp
		 */
		virtual cv::Mat annotate(const float new_threshold = -1.0f);

		/** Return the @ref duration as a text string which can then be added to the image during annotation.
		 * @see @ref annotate()
		 */
		std::string duration_string();

		/** Obtain a vector of several bright colours that may be used to annotate images.
		 * Remember that OpenCV uses BGR, not RGB.  So pure red is @p "{0, 0, 255}".
		 * @see @ref annotation_colours
		 */
		static VColours get_default_annotation_colours();

#ifdef DARKHELP_CAN_INCLUDE_DARKNET
		/** Static function to convert the OpenCV @p cv::Mat objects to Darknet's internal @p image format.
		 * Provided for convenience in case you need to call into one of Darknet's functions.
		 * @see @ref convert_darknet_image_to_opencv_mat()
		 */
		static image convert_opencv_mat_to_darknet_image(cv::Mat mat);

		/** Static function to convert Darknet's internal @p image format to OpenCV's @p cv::Mat format.
		 * Provided for convenience in case you need to manipulate a Darknet image.
		 * @see @ref convert_opencv_mat_to_darknet_image()
		 */
		static cv::Mat convert_darknet_image_to_opencv_mat(const image img);

		/** The Darknet network.  This is setup in the constructor.
		 * @note Unfortunately, the Darknet C API does not allow this to be de-allocated!
		 */
		network * net;
#else
		/** The Darknet network, but stored as a void* pointer so we don't have to include darknet.h.
		 * @note Unfortunately, the Darknet C API does not allow this to be de-allocated!
		 */
		void * net;
#endif

		/** A vector of names corresponding to the identified classes.  This is typically setup in the constructor,
		 * but can be manually set afterwards.
		 */
		VStr names;

		/** The length of time it took to initially load the network and weights (after the %DarkHelp object has been
		 * constructed), or the length of time @ref predict() took to run on the last image to be processed.
		 * @see @ref duration_string()
		 */
		std::chrono::high_resolution_clock::duration duration;

		/// Image prediction threshold.  Defaults to 0.5.  @see @ref predict()  @see @ref annotate()
		float threshold;

		/** Used during prediction.  Defaults to 0.5.  @see @ref predict()
		 * @todo Need to find more details on how this setting works in Darknet.
		 */
		float hierchy_threshold;

		/** Non-Maximal Suppression (NMS) threshold suppresses overlapping bounding boxes and only retains the bounding
		 * box that has the maximum probability of object detection associated with it.  Defaults to 0.45.
		 * @see @ref predict()
		 */
		float non_maximal_suppression_threshold;

		/// A copy of the most recent results after applying the neural network to an image.  This is set by @ref predict().
		PredictionResults prediction_results;

		/** Determines if the name given to each prediction includes the percentage.
		 *
		 * For example, the name for a prediction might be @p "dog" when this flag is set to @p false, or it might be
		 * @p "dog 98%" when set to @p true.  Defaults to true.
		 */
		bool names_include_percentage;

		/** Determine if multiple class names are included when labelling an item.
		 *
		 * For example, if an object is 95% car or 80% truck, then the label could say @p "car, truck"
		 * when this is set to @p true, and simply @p "car" when set to @p false.  Defaults to @p true.
		 */
		bool include_all_names;

		/** The colours to use in @ref annotate().  Defaults to @ref get_default_annotation_colours().
		 * 
		 * Remember that OpenCV uses BGR, not RGB.  So pure red is @p "(0, 0, 255)".
		 */
		VColours annotation_colours;

		/// Font face to use in @ref annotate().  Defaults to @p cv::HersheyFonts::FONT_HERSHEY_SIMPLEX.
		cv::HersheyFonts annotation_font_face;

		/// Scaling factor used for the font in @ref annotate().  Defaults to 0.5.
		double annotation_font_scale;

		/// Thickness of the font in @ref annotate().  Defaults to 1.
		int annotation_font_thickness;

		/** If set to @p true then @ref annotate() will call @ref duration_string() and display on the top-left of the
		 * image the length of time @ref predict() took to process the image.  Defaults to true.
		 *
		 * When enabed, the duration may look similar to this:
		 * @image html barcode_100_percent.png
		 */
		bool annotation_include_duration;

		/** If set to @p true then @ref annotate() will display a timestamp on the bottom-left corner of the image.
		 * Defaults to false.
		 *
		 * When enabled, the timestamp may look similar to this:
		 * @image html barcode_with_timestamp.png
		 */
		bool annotation_include_timestamp;

		/// The most recent image handled by @ref predict().
		cv::Mat original_image;

		/// The most recent output produced by @ref annotate().
		cv::Mat annotated_image;


	protected:

		/** @internal Used by all the other @ref predict() calls to do the actual network prediction.  This uses the
		 * image stored in @ref original_image.
		 */
		PredictionResults predict(const float new_threshold = -1.0f);
};


/** Convenience function to stream a single result as a "readable" line of text.
 * Mostly intended for debug or logging purposes.
 */
std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResult & pred);


/** Convenience function to stream an entire vector of results as readable text.
 * Mostly intended for debug or logging purposes.
 *
 * For example:
 *
 * ~~~~
 * DarkHelp darkhelp("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
 * const auto results = darkhelp.predict("test_image_01.jpg");
 * std::cout << results << std::endl;
 * ~~~~
 *
 * This would generate text similar to this:
 *
 * ~~~~{.txt}
 * prediction results: 12
 * -> 1/12: "Barcode 94%" #43 prob=0.939646 x=430 y=646 w=173 h=17 entries=1
 * -> 2/12: "Tag 100%" #40 prob=0.999954 x=366 y=320 w=281 h=375 entries=1
 * -> 3/12: "G 85%, 2 12%" #19 prob=0.846418 x=509 y=600 w=28 h=37 entries=2 [ 2=0.122151 19=0.846418 ]
 * ...
 * ~~~~
 *
 * Where:
 *
 * @li @p "1/12" is the number of predictions found.
 * @li @p "Barcode 94%" is the class name and the probability if @ref DarkHelp::names_include_percentage is enabled.
 * @li @p "#43" is the zero-based class index.
 * @li @p "prob=0.939646" is the probabilty that it is class #43.  (Multiply by 100 to get percentage.)
 * @li @p "x=..." are the X, Y, width, and height of the rectangle that was identified.
 * @li @p "entries=1" means that only 1 class was matched.  If there is more than 1 possible class,
 * then the class index and probability for each class will be shown.
 */
std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResults & results);


/** Convenience function to resize an image yet retain the exact original aspect ratio.  Performs no resizing if the
 * image is already the desired size.  Depending on the size of the original image and the desired size, a "best"
 * size will be chosen that does not exceed the specified size.
 *
 * For example, if the image is 640x480, and the specified size is 400x400, the image returned will be 400x300
 * which maintains the original 1.333 aspect ratio.
 */
cv::Mat resize_keeping_aspect_ratio(cv::Mat mat, const cv::Size & desired_size);
