package com.example.ecproject;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.ecproject.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("ECproject");
    }

    private static final String TAG = "CameraNDK";
    private static final int PERMISSION_REQUEST_CODE_CAMERA = 1;
    private ActivityMainBinding binding;

    public native void scan();
    public native void flipCamera();
    public native void setSurface(Surface surface);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        String[] requiredPermissions = { Manifest.permission.CAMERA };
        if (!hasPermissions(requiredPermissions)) {
            ActivityCompat.requestPermissions(this, requiredPermissions, PERMISSION_REQUEST_CODE_CAMERA);
        } else {
            initNativeComponents();
        }
    }

    private void initNativeComponents() {
        SurfaceView surfaceView = binding.surfaceView;
        SurfaceHolder surfaceHolder = surfaceView.getHolder();

        surfaceHolder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder) {
                Log.v(TAG, "surfaceCreated");
                // On attend surfaceChanged pour avoir les bonnes dimensions
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
                Log.v(TAG, "surfaceChanged: " + width + "x" + height);
                setSurface(holder.getSurface());
            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                Log.v(TAG, "surfaceDestroyed");
            }
        });

        binding.scanButton.setOnClickListener(v -> scan());
        binding.flipButton.setOnClickListener(v -> flipCamera());
    }

    private boolean hasPermissions(String[] permissions) {
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE_CAMERA) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initNativeComponents();
            } else {
                finish();
            }
        }
    }
}
