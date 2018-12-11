package com.huaan.usbcamerademok

interface OnFrameCallBack {
    fun onFrameCallBack(rgb : ByteArray,width : Int , height : Int ,cameraId : Int)
}