package com.example.ecproject;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.BatteryManager;
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

    private static final String TAG = "ECProject";

    private static final int PERMISSION_REQUEST_CODE = 1;
    private static final String[] REQUIRED_PERMISSIONS = {
            Manifest.permission.CAMERA,
            Manifest.permission.ACCESS_FINE_LOCATION
    };

    private ActivityMainBinding binding;

    // GPS
    private LocationManager  locationManager;
    private LocationListener locationListener;

    // Batterie
    private BroadcastReceiver batteryReceiver;

    // -------------------------------------------------------------------------
    // JNI declarations
    // -------------------------------------------------------------------------

    /** Appelé dans onResume — démarre DeviceRuntime, AuthManager, WebRtcEngine */
    public native void onStart();

    /** Appelé dans onPause — arrête caméra, WebRTC, DeviceRuntime */
    public native void onStop();

    /** Passe la surface native (appelé depuis surfaceChanged) */
    public native void setSurface(Surface surface, int width, int height);

    /** Relâche la surface native (appelé depuis surfaceDestroyed) */
    public native void releaseSurface();

    /** Bascule caméra avant/arrière */
    public native void flipCamera();

    /** Mise à jour GPS — appelé depuis LocationListener */
    public native void onGpsUpdate(
            double lat, double lon, double alt,
            float speed, float bearing, float accuracy,
            long timestampMs);

    /** Mise à jour batterie — appelé depuis BroadcastReceiver */
    public native void onBatteryUpdate(int level);

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        if (!hasPermissions(REQUIRED_PERMISSIONS)) {
            ActivityCompat.requestPermissions(this, REQUIRED_PERMISSIONS, PERMISSION_REQUEST_CODE);
        } else {
            initNativeComponents();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Démarre le runtime natif (DeviceRuntime, AuthManager, WebRtcEngine)
        onStart();
        registerBatteryReceiver();
        startGps();
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopGps();
        unregisterBatteryReceiver();
        // Arrête proprement le runtime natif
        onStop();
    }

    // -------------------------------------------------------------------------
    // Surface + boutons
    // -------------------------------------------------------------------------

    private void initNativeComponents() {
        SurfaceView surfaceView   = binding.surfaceView;
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
                setSurface(holder.getSurface(), width, height);
            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                Log.v(TAG, "surfaceDestroyed");
                releaseSurface();
            }
        });

        binding.flipButton.setOnClickListener(v -> flipCamera());
    }

    // -------------------------------------------------------------------------
    // GPS
    // -------------------------------------------------------------------------

    private void startGps() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            Log.w(TAG, "startGps: permission ACCESS_FINE_LOCATION absente");
            return;
        }

        locationManager  = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        locationListener = new LocationListener() {
            @Override
            public void onLocationChanged(@NonNull Location loc) {
                onGpsUpdate(
                        loc.getLatitude(),
                        loc.getLongitude(),
                        loc.hasAltitude()  ? loc.getAltitude()  : 0.0,
                        loc.hasSpeed()     ? loc.getSpeed()     : 0.0f,
                        loc.hasBearing()   ? loc.getBearing()   : 0.0f,
                        loc.hasAccuracy()  ? loc.getAccuracy()  : 0.0f,
                        loc.getTime()
                );
            }

            @Override public void onStatusChanged(String provider, int status, Bundle extras) {}
            @Override public void onProviderEnabled(@NonNull String provider)  {}
            @Override public void onProviderDisabled(@NonNull String provider) {
                Log.w(TAG, "GPS provider désactivé: " + provider);
            }
        };

        // GPS en priorité, fallback réseau si indisponible
        if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)) {
            locationManager.requestLocationUpdates(
                    LocationManager.GPS_PROVIDER,
                    1000L,   // minTime ms
                    1.0f,    // minDistance mètres
                    locationListener
            );
            Log.i(TAG, "GPS démarré (provider: GPS)");
        } else if (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER)) {
            locationManager.requestLocationUpdates(
                    LocationManager.NETWORK_PROVIDER,
                    1000L,
                    1.0f,
                    locationListener
            );
            Log.w(TAG, "GPS indispo — fallback réseau");
        } else {
            Log.e(TAG, "Aucun provider de localisation disponible");
        }
    }

    private void stopGps() {
        if (locationManager != null && locationListener != null) {
            locationManager.removeUpdates(locationListener);
            locationListener = null;
            Log.i(TAG, "GPS arrêté");
        }
    }

    // -------------------------------------------------------------------------
    // Batterie
    // -------------------------------------------------------------------------

    private void registerBatteryReceiver() {
        batteryReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                int level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                int scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, 100);
                int pct   = (scale > 0) ? (int)((level / (float) scale) * 100) : -1;
                onBatteryUpdate(pct);
            }
        };
        IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        registerReceiver(batteryReceiver, filter);
        Log.i(TAG, "BatteryReceiver enregistré");
    }

    private void unregisterBatteryReceiver() {
        if (batteryReceiver != null) {
            unregisterReceiver(batteryReceiver);
            batteryReceiver = null;
            Log.i(TAG, "BatteryReceiver désenregistré");
        }
    }

    // -------------------------------------------------------------------------
    // Permissions
    // -------------------------------------------------------------------------

    private boolean hasPermissions(String[] permissions) {
        for (String p : permissions) {
            if (ContextCompat.checkSelfPermission(this, p) != PackageManager.PERMISSION_GRANTED)
                return false;
        }
        return true;
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode,
            @NonNull String[] permissions,
            @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            boolean allGranted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }
            if (allGranted) {
                initNativeComponents();
            } else {
                Toast.makeText(this,
                        "Permissions requises : caméra et localisation",
                        Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }
}
