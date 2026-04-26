package de.safeturn.cyclist;

import android.Manifest;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Locale;
import java.util.Random;

public class MainActivity extends Activity {
    private static final int REQ_PERMISSIONS = 1201;
    private static final int TEST_COMPANY_ID = 0xFFFF;

    private BluetoothLeAdvertiser advertiser;
    private AdvertiseCallback advertiseCallback;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean advertising = false;
    private int seq = 0;
    private int tempId;

    private float speedKmh = 0f;
    private int headingDeg = 90;
    private int accuracyM = 255;

    private TextView statusView;
    private TextView liveView;
    private Button startButton;
    private EditText manualSpeed;
    private EditText manualHeading;

    private LocationManager locationManager;
    private Location lastLocation;

    private final Runnable tick = new Runnable() {
        @Override public void run() {
            if (advertising) {
                restartAdvertisingPacket();
                handler.postDelayed(this, 1000);
            }
        }
    };

    private final LocationListener locationListener = new LocationListener() {
        @Override public void onLocationChanged(Location location) {
            if (lastLocation != null && location.distanceTo(lastLocation) > 1.5f) {
                float bearing = lastLocation.bearingTo(location);
                if (!Float.isNaN(bearing)) headingDeg = normalizeHeading(Math.round(bearing));
            } else if (location.hasBearing()) {
                headingDeg = normalizeHeading(Math.round(location.getBearing()));
            }
            if (location.hasSpeed()) speedKmh = Math.max(0f, location.getSpeed() * 3.6f);
            if (location.hasAccuracy()) accuracyM = Math.min(255, Math.max(0, Math.round(location.getAccuracy())));
            lastLocation = location;
            updateLiveText();
        }

        @Override public void onProviderEnabled(String provider) {}
        @Override public void onProviderDisabled(String provider) {}
    };

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        tempId = new Random().nextInt(0xFFFF);
        buildUi();
        requestNeededPermissions();
        initBluetooth();
        initLocation();
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        stopAdvertising();
        if (locationManager != null) locationManager.removeUpdates(locationListener);
    }

    private void buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(36, 48, 36, 36);
        root.setGravity(Gravity.CENTER_HORIZONTAL);

        TextView title = new TextView(this);
        title.setText("SafeTurn Fahrrad-App");
        title.setTextSize(26f);
        title.setGravity(Gravity.CENTER);
        title.setTypeface(null, 1);
        root.addView(title, new LinearLayout.LayoutParams(-1, -2));

        TextView subtitle = new TextView(this);
        subtitle.setText("Sendet lokale BLE-Signale an das LKW-Gateway. Keine Cloud, kein Mobilfunk.");
        subtitle.setTextSize(16f);
        subtitle.setGravity(Gravity.CENTER);
        subtitle.setPadding(0, 10, 0, 22);
        root.addView(subtitle, new LinearLayout.LayoutParams(-1, -2));

        statusView = new TextView(this);
        statusView.setText("Status: bereit");
        statusView.setTextSize(18f);
        statusView.setPadding(0, 8, 0, 12);
        root.addView(statusView, new LinearLayout.LayoutParams(-1, -2));

        liveView = new TextView(this);
        liveView.setTextSize(16f);
        liveView.setPadding(0, 8, 0, 18);
        root.addView(liveView, new LinearLayout.LayoutParams(-1, -2));

        manualSpeed = new EditText(this);
        manualSpeed.setHint("Geschwindigkeit km/h, optional z.B. 18");
        manualSpeed.setInputType(android.text.InputType.TYPE_CLASS_NUMBER | android.text.InputType.TYPE_NUMBER_FLAG_DECIMAL);
        root.addView(manualSpeed, new LinearLayout.LayoutParams(-1, -2));

        manualHeading = new EditText(this);
        manualHeading.setHint("Richtung Grad, optional z.B. 90");
        manualHeading.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
        root.addView(manualHeading, new LinearLayout.LayoutParams(-1, -2));

        Button applyManual = new Button(this);
        applyManual.setText("Manuelle Werte übernehmen");
        applyManual.setOnClickListener(v -> applyManualValues());
        root.addView(applyManual, new LinearLayout.LayoutParams(-1, -2));

        startButton = new Button(this);
        startButton.setText("BLE senden starten");
        startButton.setTextSize(18f);
        startButton.setOnClickListener(v -> {
            if (advertising) stopAdvertising(); else startAdvertising();
        });
        root.addView(startButton, new LinearLayout.LayoutParams(-1, -2));

        TextView note = new TextView(this);
        note.setText("Hinweis: Die App muss für diesen MVP aktiv laufen. Für ein Produkt später Foreground Service/SafetyTag nutzen.");
        note.setTextSize(14f);
        note.setPadding(0, 28, 0, 0);
        root.addView(note, new LinearLayout.LayoutParams(-1, -2));

        setContentView(root);
        updateLiveText();
    }

    private void requestNeededPermissions() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return;
        String[] permissions;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions = new String[] {
                Manifest.permission.BLUETOOTH_ADVERTISE,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_FINE_LOCATION
            };
        } else {
            permissions = new String[] { Manifest.permission.ACCESS_FINE_LOCATION };
        }
        requestPermissions(permissions, REQ_PERMISSIONS);
    }

    private boolean hasPermission(String permission) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return true;
        return checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
    }

    private void initBluetooth() {
        BluetoothManager manager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = manager != null ? manager.getAdapter() : null;
        if (adapter == null) {
            statusView.setText("Status: Bluetooth nicht verfügbar");
            return;
        }
        if (!adapter.isEnabled()) {
            statusView.setText("Status: Bluetooth ist deaktiviert");
            return;
        }
        advertiser = adapter.getBluetoothLeAdvertiser();
        if (advertiser == null) {
            statusView.setText("Status: BLE Advertising wird auf diesem Gerät nicht unterstützt");
        }
    }

    private void initLocation() {
        locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        if (locationManager == null) return;
        if (!hasPermission(Manifest.permission.ACCESS_FINE_LOCATION)) return;
        try {
            locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 1000, 1.0f, locationListener);
            locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 1500, 1.0f, locationListener);
        } catch (SecurityException ignored) {}
    }

    private void applyManualValues() {
        try {
            String s = manualSpeed.getText().toString().trim();
            if (!s.isEmpty()) speedKmh = Float.parseFloat(s.replace(',', '.'));
            String h = manualHeading.getText().toString().trim();
            if (!h.isEmpty()) headingDeg = normalizeHeading(Integer.parseInt(h));
            updateLiveText();
            if (advertising) restartAdvertisingPacket();
        } catch (Exception ex) {
            Toast.makeText(this, "Ungültige manuelle Werte", Toast.LENGTH_SHORT).show();
        }
    }

    private void startAdvertising() {
        if (advertiser == null) {
            initBluetooth();
            if (advertiser == null) return;
        }
        advertising = true;
        startButton.setText("BLE senden stoppen");
        statusView.setText("Status: BLE Advertising aktiv");
        handler.post(tick);
    }

    private void stopAdvertising() {
        advertising = false;
        handler.removeCallbacks(tick);
        if (advertiser != null && advertiseCallback != null) {
            try { advertiser.stopAdvertising(advertiseCallback); } catch (Exception ignored) {}
        }
        advertiseCallback = null;
        if (startButton != null) startButton.setText("BLE senden starten");
        if (statusView != null) statusView.setText("Status: gestoppt");
    }

    private void restartAdvertisingPacket() {
        if (advertiser == null) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && !hasPermission(Manifest.permission.BLUETOOTH_ADVERTISE)) {
            statusView.setText("Status: BLUETOOTH_ADVERTISE Berechtigung fehlt");
            return;
        }

        if (advertiseCallback != null) {
            try { advertiser.stopAdvertising(advertiseCallback); } catch (Exception ignored) {}
        }

        AdvertiseSettings settings = new AdvertiseSettings.Builder()
            .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
            .setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH)
            .setConnectable(false)
            .setTimeout(0)
            .build();

        // Keep the legacy advertisement below 31 bytes. Therefore no 128-bit Service UUID here.
        AdvertiseData data = new AdvertiseData.Builder()
            .setIncludeDeviceName(false)
            .setIncludeTxPowerLevel(false)
            .addManufacturerData(TEST_COMPANY_ID, buildPayload())
            .build();

        advertiseCallback = new AdvertiseCallback() {
            @Override public void onStartSuccess(AdvertiseSettings settingsInEffect) {
                statusView.setText("Status: sendet BLE, Seq " + seq);
            }
            @Override public void onStartFailure(int errorCode) {
                statusView.setText("Status: Advertising Fehler " + errorCode);
            }
        };

        try {
            advertiser.startAdvertising(settings, data, advertiseCallback);
        } catch (SecurityException se) {
            statusView.setText("Status: Bluetooth-Berechtigung fehlt");
        } catch (Exception ex) {
            statusView.setText("Status: Advertising fehlgeschlagen: " + ex.getMessage());
        }

        updateLiveText();
    }

    private byte[] buildPayload() {
        seq = (seq + 1) & 0xFF;
        int speedHalf = Math.max(0, Math.min(255, Math.round(speedKmh * 2f)));
        int headingHalf = Math.max(0, Math.min(179, normalizeHeading(headingDeg) / 2));
        int acc = Math.max(0, Math.min(255, accuracyM));

        ByteBuffer b = ByteBuffer.allocate(12).order(ByteOrder.LITTLE_ENDIAN);
        b.put((byte) 'S');
        b.put((byte) 'T');
        b.put((byte) 1);      // version
        b.put((byte) 1);      // role: cyclist
        b.put((byte) 1);      // flags: GPS/position data available-ish
        b.put((byte) seq);
        b.put((byte) speedHalf);
        b.put((byte) headingHalf);
        b.put((byte) acc);
        b.put((byte) 255);    // battery unknown
        b.putShort((short) tempId);
        return b.array();
    }

    private void updateLiveText() {
        String text = String.format(Locale.GERMANY,
            "Temp-ID: %04X\nGeschwindigkeit: %.1f km/h\nRichtung: %d°\nGenauigkeit: %s\nSeq: %d",
            tempId,
            speedKmh,
            headingDeg,
            accuracyM == 255 ? "unbekannt" : accuracyM + " m",
            seq
        );
        if (liveView != null) liveView.setText(text);
    }

    private int normalizeHeading(int heading) {
        int h = heading % 360;
        return h < 0 ? h + 360 : h;
    }
}
