
#include "multiStreamUtility.hpp"

STREAM_BUFFER_HANDLE streamBufferHandle[MAXSTREAMS][MAXBUFFERS] = {INVALID_STREAM_BUFFER_HANDLE};
STREAM_HANDLE camStreamHandle[2] = {0};

int frameLeftCounter = 0;
int frameRightCounter = 0;
cv::Mat frameInpLeft;
cv::Mat frameInpRight;
cv::Mat fullFrame;

void CameraRightCallback(STREAM_BUFFER_HANDLE streamBufferHandle, void* userContext)
{
    if (streamBufferHandle == 0)
    {
        DEBUG("Received NULL on stream buffer handle.");
        return;
    }

    // as a minimum, application needs to get pointer to current frame memory
    unsigned char* frMemPtr = nullptr;
    KY_DATA_TYPE frType = KY_DATATYPE_SIZET;
    size_t frSize = 8;    
    uint32_t frameId = 0;

    KYFG_BufferGetInfo(streamBufferHandle, KY_STREAM_BUFFER_INFO_BASE, &frMemPtr, &frSize, &frType);
    kayaErrchk(KYFG_BufferGetInfo(streamBufferHandle, KY_STREAM_BUFFER_INFO_ID, &frameId, NULL, NULL));
    DEBUG("Right Frame: " << "\n\tFrame Ptr: " << (size_t)frMemPtr << "\n\tFrame Id: " << frameId << "\n\tFrame DataType: " << frType << "\n\tFrame Size: " << frSize);
    
    frameInpRight = cv::Mat(1098, 1920, CV_16U, frMemPtr);
    imshow("Frame Inp Right", frameInpRight);
    cv::waitKey(10);
    /* char fileName[100] = {0};
    sprintf(fileName,"./Frames/R%0.4d.png",frameRightCounter);
    cv::imwrite(fileName,frameInpRight);
    frameRightCounter++; */
    
    // return stream buffer to input queue
    KYFG_BufferToQueue(streamBufferHandle, KY_ACQ_QUEUE_INPUT);
}

void CameraLeftCallback(STREAM_BUFFER_HANDLE streamBufferHandle, void* userContext)
{
    if (streamBufferHandle == 0)
    {
        DEBUG("Received NULL on stream buffer handle.");
        return;
    }
            
    // as a minimum, application needs to get pointer to current frame memory
    unsigned char* frMemPtr = nullptr;
    KY_DATA_TYPE frType = KY_DATATYPE_SIZET;
    size_t frSize = 8;
    uint32_t frameId = 0;

    KYFG_BufferGetInfo(streamBufferHandle, KY_STREAM_BUFFER_INFO_BASE, &frMemPtr, &frSize, &frType);
    kayaErrchk(KYFG_BufferGetInfo(streamBufferHandle, KY_STREAM_BUFFER_INFO_ID, &frameId, NULL, NULL));
    DEBUG("Left Frame: " << "\n\tFrame Ptr: " << (size_t)frMemPtr << "\n\tFrame Id: " << frameId << "\n\tFrame DataType: " << frType << "\n\tFrame Size: " << frSize);
    
    frameInpLeft = cv::Mat(1098, 1920, CV_16U, frMemPtr);
    imshow("Frame Inp Left", frameInpLeft);
    cv::waitKey(10);
    /* char fileName[100] = {0};
    sprintf(fileName,"./Frames/L%0.4d.png",frameLeftCounter);
    cv::imwrite(fileName,frameInpLeft);
    frameLeftCounter++; */

    // return stream buffer to input queue
    KYFG_BufferToQueue(streamBufferHandle, KY_ACQ_QUEUE_INPUT);
}

int main(int argc, char* argv[])
{
    /* Library Initialization */
    KYFGLib_InitParameters kyInit;
    kyInit.version = 2;
    kyInit.concurrency_mode = 0;
    kyInit.logging_mode = 0;
    kayaErrchk(KYFGLib_Initialize(&kyInit));

    /* Scan for devices connected */
    int numDevices = 0;
    kayaErrchk(KY_DeviceScan(&numDevices));

    /* Get info about connected devices */
    FGHANDLE* fgHandlePtr = nullptr;
    KY_DEVICE_INFO* devInfoArray = nullptr;
    fgHandlePtr = (FGHANDLE*) malloc(numDevices*sizeof(FGHANDLE));
    devInfoArray = (KY_DEVICE_INFO*) malloc(numDevices*sizeof(KY_DEVICE_INFO));
    int physicalFGHandleIdx = -1;
    
    DEBUG(numDevices << " Available Devices: Name {PID, isVirtual}\n");
    for (int i=0; i<numDevices; i++)
    {
        fgHandlePtr[i] = i;
        devInfoArray[i].version = KY_MAX_DEVICE_INFO_VERSION;
        kayaErrchk(KY_DeviceInfo(fgHandlePtr[i], &devInfoArray[i]));
        DEBUG(devInfoArray[i].szDeviceDisplayName << " {" << devInfoArray[i].DevicePID << ',' << (int)devInfoArray[i].isVirtual << "}\n");
        if (!devInfoArray[i].isVirtual)
            physicalFGHandleIdx = i;
    }

    /* Connect to the physical device */
    FGHANDLE physicalFGHandle = KYFG_Open(physicalFGHandleIdx);
    int64_t queuedBufferCapable = KYFG_GetGrabberValueInt(physicalFGHandle, DEVICE_QUEUED_BUFFERS_SUPPORTED);
    DEBUG("Is Queue Buffer Capable? : " << queuedBufferCapable);
    assertChk(queuedBufferCapable == 1, "Queue Buffers are not supported");

    /* Check for connected cameras */
    CAMHANDLE camHandleArr[KY_MAX_CAMERAS] = {0};
    int numCamerasDetected = 0;
    kayaErrchk(KYFG_CameraScan(physicalFGHandle, camHandleArr, &numCamerasDetected));
    DEBUG("Num cameras detected: " << numCamerasDetected);

    KYFG_WritePortReg(physicalFGHandle,0,0x6034,swap_uint32(1)) ;//AcqStart
    KYFG_WritePortReg(physicalFGHandle,0,0x601C,0) ;//AcqStart
    KYFG_WritePortReg(physicalFGHandle,0,0x601C,0) ;//AcqStart
    KYFG_WritePortReg(physicalFGHandle,0,0x4008,swap_uint32(0xde)) ;//(VIS) Master Host Link ID
    KYFG_WritePortReg(physicalFGHandle,0,0x4010,swap_uint32(0x00000400)) ;//StreamSize     
    KYFG_WritePortReg(physicalFGHandle,0,0x601C,swap_uint32(0x00000000)) ;//AcqStart
    KYFG_WritePortReg(physicalFGHandle,0,0x6008,swap_uint32(0x00000323)) ;//PortPixelFormat := $333 ;//BayerGB12
    KYFG_WritePortReg(physicalFGHandle,0,0x6024,swap_uint32(0x42700000)) ;//PortFrameRate :=  ;//60fps
    KYFG_WritePortReg(physicalFGHandle,0,0x6000,swap_uint32(1920)) ;//PortWidth := 1920 ;
    KYFG_WritePortReg(physicalFGHandle,0,0x6004,swap_uint32(1098)) ;//PortHeight := 1098 ;//Camera sends 1920x1098     
    KYFG_WritePortReg(physicalFGHandle,0,0x6028,0) ;//PortSensorControl0 := $0 ;
    KYFG_WritePortReg(physicalFGHandle,0,0x602c,0) ;//PortSensorControl1 := $0 ;
    KYFG_WritePortReg(physicalFGHandle,0,0x6018,0) ;//PortAcqMode := 0 ;
    KYFG_WritePortReg(physicalFGHandle,0,0x6030,swap_uint32(0)) ; //1=Test Pattern

    /* Modifying the frame grabber parameters for dual-streaming */
    static const uint64_t cameraConnectionSpeedRegAddr = 0x4014;
    uint32_t cameraConnectionSpeedRegValue = 0;
    kayaErrchk(KYFG_ReadPortReg(physicalFGHandle, 0, cameraConnectionSpeedRegAddr, &cameraConnectionSpeedRegValue)); 
    cameraConnectionSpeedRegValue = swap_uint32(cameraConnectionSpeedRegValue); // BigEndian to LittleEndian

    // setup manual camera detection to create multiple virtual streams
    for(int iCamera = 0; iCamera < MAXSTREAMS; iCamera++)
    {
        static const uint64_t fgMultistreamRegBase = 0x402064;
        static const uint64_t fgMultistreamRegStep = 0x1000;
        const uint32_t fgMultistreamRegValue = 0;
        kayaErrchk(KYFG_DeviceDirectHardwareWrite(physicalFGHandle, fgMultistreamRegBase + fgMultistreamRegStep * iCamera, &fgMultistreamRegValue, sizeof(fgMultistreamRegValue)));
        kayaErrchk(KYFG_SetGrabberValueInt(physicalFGHandle, "CameraSelector", iCamera));
        kayaErrchk(KYFG_SetGrabberValueEnum_ByValueName(physicalFGHandle, "ManualCameraMode", "On"));
        kayaErrchk(KYFG_SetGrabberValueEnum(physicalFGHandle, "ManualCameraConnectionConfig", cameraConnectionSpeedRegValue));
        kayaErrchk(KYFG_SetGrabberValueInt(physicalFGHandle, "ManualCameraChannelSelector", 0));
        kayaErrchk(KYFG_SetGrabberValueInt(physicalFGHandle, "ManualCameraFGLink", 0));
        // // set image stream id manually
        kayaErrchk(KYFG_SetGrabberValueInt(physicalFGHandle, "Image1StreamID", iCamera + 1)); // stream0_id = 1, stream1_id = 2
    }

    kayaErrchk(KYFG_CameraScan(physicalFGHandle, camHandleArr, &numCamerasDetected));
    DEBUG("Num cameras detected after manual connection mode: " << numCamerasDetected);
    assertChk(numCamerasDetected == 2, "Stereo camera system not detected");

    /* Setting camera parameters */
    for (int camIter = 0; camIter < numCamerasDetected; camIter++)
    {
        kayaErrchk(KYFG_CameraOpen2(camHandleArr[camIter], NULL));
        kayaErrchk(KYFG_SetGrabberValueEnum_ByValueName(physicalFGHandle, "TransferControlMode", "UserControlled")); // set user controller camera acquisition mode
        
        KYFG_SetCameraValueInt(camHandleArr[camIter], "CameraSelector", camIter);

        // create stream and assign appropriate runtime acquisition callback function
        kayaErrchk(KYFG_StreamCreate(camHandleArr[camIter], &camStreamHandle[camIter], 0));
        // kayaErrchk(KYFG_StreamBufferCallbackRegister(camStreamHandle[camIter], Stream_callback_func, &camStreamHandle[camIter])); // pass stream handle as an argument returned in stream callback

        // Retrieve information about required frame buffer size and alignment 
        size_t frameDataSize, frameDataAligment;
        kayaErrchk(KYFG_StreamGetInfo(camStreamHandle[camIter],KY_STREAM_INFO_PAYLOAD_SIZE,&frameDataSize,NULL, NULL));
        kayaErrchk(KYFG_StreamGetInfo(camStreamHandle[camIter],KY_STREAM_INFO_BUF_ALIGNMENT,&frameDataAligment,NULL, NULL));

        // Allocate memory for frames
        for (int iFrame = 0; iFrame < MAXBUFFERS; iFrame++)
        {
            kayaErrchk(KYFG_BufferAllocAndAnnounce(camStreamHandle[camIter], frameDataSize, NULL, &streamBufferHandle[camIter][iFrame]));
        }
    }

    kayaErrchk(KYFG_StreamBufferCallbackRegister(camStreamHandle[0], CameraLeftCallback, NULL)); // pass stream handle as an argument returned in stream callback
    kayaErrchk(KYFG_StreamBufferCallbackRegister(camStreamHandle[1], CameraRightCallback, NULL)); // pass stream handle as an argument returned in stream callback

    /* Start camera aquisition */
    // kayaErrchk(KYFG_SetCameraValueInt(camHandleArr[0], "CameraSelector", 0));
    kayaErrchk(KYFG_BufferQueueAll(camStreamHandle[0], KY_ACQ_QUEUE_UNQUEUED, KY_ACQ_QUEUE_INPUT));
    KYFG_CameraStart(camHandleArr[0], camStreamHandle[0], 0);
    kayaErrchk(KYFG_CameraExecuteCommand(camHandleArr[0], "AcquisitionStart"));

    // kayaErrchk(KYFG_SetCameraValueInt(camHandleArr[1], "CameraSelector", 1));
    kayaErrchk(KYFG_BufferQueueAll(camStreamHandle[1], KY_ACQ_QUEUE_UNQUEUED, KY_ACQ_QUEUE_INPUT));
    KYFG_CameraStart(camHandleArr[1], camStreamHandle[1], 0);
    kayaErrchk(KYFG_CameraExecuteCommand(camHandleArr[1], "AcquisitionStart"));

    KYFG_WritePortReg(physicalFGHandle,0,0x601C,swap_uint32(1)); // Start Physical camera
    DEBUG("Frame Loop Running");

    /* Creating Video Writer Object */
    int frameFormat = cv::VideoWriter::fourcc('H','F','Y','U');
    cv::VideoWriter vidObj;
    vidObj.open("TestVid.avi", frameFormat, 60.0, cv::Size(1920,1098), false);
    
    signal(SIGINT, inthand);
    while(!stop)
    {
        // Put all buffers to input queue
                
        // kayaErrchk(KYFG_CameraStop(camHandleArr[0]));
        // kayaErrchk(KYFG_CameraStop(camHandleArr[1]));

        /* char c = std::cin.get();
        if (c == 'q' || c == 'Q')
            break; */
        
    }
    cv::destroyAllWindows();
    vidObj.release();

    /* Stop camera acquisition */
    for (int cameraIndex = 0; cameraIndex < numCamerasDetected; ++cameraIndex)
    {
        // stop all streams coming from the camera
        // kayaErrchk(KYFG_SetCameraValueInt(camHandleArr[cameraIndex], "CameraSelector", cameraIndex));
        kayaErrchk(KYFG_CameraExecuteCommand(camHandleArr[cameraIndex], "AcquisitionStop"));
        kayaErrchk(KYFG_CameraStop(camHandleArr[cameraIndex]));
        DEBUG("Acquisition stopped on camera: " << cameraIndex);
    }

    kayaErrchk(KYFG_Close(physicalFGHandle));
    DEBUG("Closing camera and FG. Exiting!");
    /* Free heap */
    free(fgHandlePtr);
    free(devInfoArray);
    return 0;
}