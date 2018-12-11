package com.huaan.usbcamerademok

import android.app.Activity
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.view.Surface
import com.huaan.usbcamerademok.camera.UsbCamera
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }

    }

    lateinit var usbCamera: UsbCamera
    lateinit var usbCamera2: UsbCamera

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        initCamera()
    }

    fun initCamera() {
        usbCamera = UsbCamera()
        usbCamera2 = UsbCamera()

        usbCamera.initCamera(0,0,640,480)
        usbCamera.prepareCamera()
        usbCamera.onPreview(textureView1)

        usbCamera2.initCamera(1,1,320,240)
        usbCamera2.prepareCamera()
        usbCamera2.onPreview(textureView2)
    }

    override fun onDestroy() {
        super.onDestroy()
        usbCamera2.onStopCamera(1)
    }

}
