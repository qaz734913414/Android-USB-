package com.huaan.usbcamerademok.camera

import android.graphics.Bitmap
import android.graphics.Rect
import android.graphics.SurfaceTexture
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.TextureView
import com.huaan.usbcamerademok.OnFrameCallBack
import com.huaan.usbcamerademok.utils.CMDUtils
import com.huaan.usbcamerademok.utils.FileUtils.rgb2Bitmap

public class UsbCamera {

    private val TAG = "UsbCamera"

    //JNI


        private external fun prepareCameraWithBase(cameraId: Int, camerabase: Int, weidth: Int, height: Int): Int

        private external fun stopCamera(videobase: Int)

        private external fun getYUVData(bytes: ByteArray, index: Int): Int

        private external fun yuyv2rgb24(yuvByte: ByteArray, rgbByte: ByteArray, width: Int, height: Int)





    private var mWidth = 640
    private var mHeight = 480
    private var mCameraId = 0
    private var mCameraBase = -1

    private var rgbByte: ByteArray? = null
    private var yuvByte: ByteArray? = null
    private var drawBitmap: Bitmap? = null
    private var winWidth = 0
    private var winHeight = 0
    private var dw: Int = 0
    private var dh:Int = 0
    private var rate: Float = 0.toFloat()
    private var rect: Rect? = null

    fun initCamera(cameraId: Int, camerabase: Int, width: Int, height: Int) {
        //开启权限
        CMDUtils.executeCMD("chmod 777 /dev/video*")
        //判断相机数量
        this.mCameraId = cameraId
        this.mWidth = width
        this.mHeight = height
        this.mCameraBase = camerabase
    }

    fun prepareCamera(): Int {
        rgbByte = ByteArray(mWidth * mHeight * 3)
        yuvByte = ByteArray(mWidth * mHeight * 2)
        val ret = prepareCameraWithBase(mCameraId, mCameraBase, mWidth, mHeight)
        Log.e(TAG, "prepareCamera: ret = " + ret + "index = " + mCameraBase)
        if (ret == 0) {
            Thread( Runnable {
                while (true) {
                    val size = getYUVData(yuvByte!!, mCameraBase)
                    if (size <= 0) {
                        continue
                    }

                    synchronized(rgbByte!!) {
                        val start = System.currentTimeMillis()
                        yuyv2rgb24(yuvByte!!, rgbByte!!, mWidth, mHeight)
                        Log.i(TAG, "run: yuyv2rgb" + (System.currentTimeMillis() - start))
                    }

                    if (onFrameCallBack != null) {
                        onFrameCallBack!!.onFrameCallBack(rgbByte!!, mWidth, mHeight, mCameraId)
                    }
                    try {
                        Thread.sleep(1)
                    } catch (e: InterruptedException) {
                        e.printStackTrace()
                    }

                }
            }).start()
        }
        return ret
    }


    private var onFrameCallBack: OnFrameCallBack? = null

    fun setOnFrameCallBack(callBack: OnFrameCallBack) {
        this.onFrameCallBack = callBack
    }

    fun onPreview(surface: TextureView) {

        checkWHforVideo()

        Thread(Runnable {
            while (true) {
                synchronized(this.rgbByte!!) {
                    val start = System.currentTimeMillis()
                    drawBitmap = rgb2Bitmap(rgbByte, mWidth, mHeight)
                    Log.i(TAG, "run: rgb2Bitmap" + (System.currentTimeMillis() - start))
                }
                val canvas = surface.lockCanvas(null)
                if (canvas != null) {
                    // draw camera bmp on canvas
                    canvas.drawBitmap(drawBitmap, null, rect, null)
                    surface.unlockCanvasAndPost(canvas)
                }
                try {
                    Thread.sleep(1)
                } catch (e: InterruptedException) {
                    e.printStackTrace()
                }

            }
        }).start()
    }

    /**
     * 确定画布显示大小
     */
    private fun checkWHforVideo() {
        if (winWidth == 0) {
            winWidth = mWidth
            winHeight = mHeight

            if (winWidth * 3 / 4 <= winHeight) {
                dw = 0
                dh = (winHeight - winWidth * 3 / 4) / 2
                rate = winWidth.toFloat() / mWidth
                rect = Rect(dw, dh, dw + winWidth - 1, dh + winWidth * 3 / 4 - 1)
            } else {
                dw = (winWidth - winHeight * 4 / 3) / 2
                dh = 0
                rate = winHeight.toFloat() / mHeight
                rect = Rect(dw, dh, dw + winHeight * 4 / 3 - 1, dh + winHeight - 1)
            }
        }
    }

    fun onPreview(holder: SurfaceHolder) {

        checkWHforVideo()

        Thread(Runnable {
            while (true) {
                synchronized(this.rgbByte!!) {
                    val start = System.currentTimeMillis()
                    drawBitmap = rgb2Bitmap(rgbByte, mWidth, mHeight)
                    Log.i(TAG, "run: rgb2Bitmap" + (System.currentTimeMillis() - start))
                }
                val canvas = holder.lockCanvas()
                if (canvas != null) {
                    // draw camera bmp on canvas
                    canvas.drawBitmap(drawBitmap, null, rect, null)
                    holder.unlockCanvasAndPost(canvas)
                }
                try {
                    Thread.sleep(1)
                } catch (e: InterruptedException) {
                    e.printStackTrace()
                }

            }
        }).start()

    }

    /**
     * 关闭相机
     * @param videobase 相机索引，初始化一次为0，两次1，依次递增
     */
    fun onStopCamera(videobase: Int) {
        stopCamera(videobase)
    }

}