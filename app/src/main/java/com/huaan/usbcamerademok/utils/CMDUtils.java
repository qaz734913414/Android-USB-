package com.huaan.usbcamerademok.utils;

import android.util.Log;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class CMDUtils {
    private static String TAG = "CMDUtils";

    //判断是否root
    public static boolean isRoot() {
        boolean isRoot = true;
        try {

            Runtime.getRuntime().exec("su");
        } catch (IOException e) {
            e.printStackTrace();
            isRoot = false;
        }
        Log.d(TAG, "isRoot：" + isRoot);
        return isRoot;
    }

    //执行命令，并获得结果
    public static String executeCMD(String command) {
        StringBuilder result = new StringBuilder();
        DataOutputStream dos = null;
        DataInputStream dis = null;
        //
        try {
            Process process = Runtime.getRuntime().exec("su");//提权
            dos = new DataOutputStream(process.getOutputStream());
            dis = new DataInputStream(process.getInputStream());
            dos.writeBytes(command + "\n"); //写入cmd
            dos.flush(); //清空数据
            dos.writeBytes("exit\n");
            dos.flush();
            String line = null;
            while ((line = dis.readLine()) != null) {
                Log.d(TAG + "result", line);
                result.append(line).append("\n");
            }
            process.waitFor();
        } catch (IOException | InterruptedException e) {
            Log.e(TAG, String.valueOf(e));
        }finally {
            if (dos != null) {
                try {
                    dos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (dis != null) {
                try {
                    dis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return String.valueOf(result);
    }
}
