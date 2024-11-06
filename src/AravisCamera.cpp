
//constructor 
AravisCamera::AravisCamera(){
    std::cout<<"connecting to camera\n";
    // checking for cameras
    arvcamera = arv_camera_new(NULL, &error);
    if (ARV_IS_CAMERA(arvCamera) == 0){
        throw std::runtime_error("No camera connected");
        //error if no cameras found
    }
    set_default();
    //set_epoch();
    // apparently this is unecessary because we can get timestamp for each buffer
}

//destructor
AravisCamera::~AravisCamera(){
    std::cout<<"Cleaning up\n";
    g_clear_object (&arvCamera);
}

//setting pixel format
void set_pixelformat(const std::string &pixelformat){
    const char* cstr = pixelformat.c_str();
    arv_camera_set_pixel_format_from_string (arvCamera,cstr,error);
}

//setting exposure time
void set_exposuretime(float exposuretime){
    arv_camera_set_exposure_time(arvCamera, exposuretime, error);
}

//setting gain
void set_gain(float gain) {
    std::cout << "Setting gain to " << gain << "\n";
    arv_camera_set_gain(arvCamera, gain, error);
}

// start stream
// ? Not sure about number of buffers

void start_stream (){
    std::cout << "Starting stream\n";
    stream = arv_camera_create_stream(arvCamera, NULL, NULL, error);
    if (stream == NULL){
        throw std::runtime_error("No stream");
    }
    arv_camera_start_acquisition(arvCamera, error);
}


//end acquisition and exit stream


void stop_steam(){
    std::cout << "stopping stream\n";
    arv_camera_stop_acquisiton(arvCamera,error);
    g_clear_object (&stream);
    g_clear_object (&arvBuffer);
}


//getting an image
void get_image (){
    arvBuffer = arv_camera_acquisition(arvCamera, 0, error);
    if (arvBuffer != NULL){
        //double width = arv_buffer_get_image_width(buffer);
        timeStamp = arv_buffer_get_timestamp(arvBuffer); // not sure if this one is better than the next line
        timeStamp = arv_buffer_get_system_timestamp(arvBuffer);
    }
    
    //store buffer to file commands here..?
    //store timestamp as well?
    g_clear_object (&arvBuffer);
    g_clear_object(&timeStamp);
}

void set_epoch(){

    //is this still necessary if we can find timestamp of buffer?
}

//setting default values to camera 
void set_default(){
    
    //const char* string is the name of the node based on GenIcam standard naming
    arv_gc_set_default_node_data (genicam, "ExposureTime");
    arv_gc_set_default_node_data (genicam, "PixelFormat");
    arv_gc_set_default_node_data (genicam, "Gain");

}

