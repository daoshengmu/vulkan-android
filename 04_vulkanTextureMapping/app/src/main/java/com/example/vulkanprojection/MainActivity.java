package com.example.vulkanTextureMapping;

import androidx.appcompat.app.AppCompatActivity;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.example.vulkanTextureMapping.R;

import java.io.File;

import static android.Manifest.permission.READ_EXTERNAL_STORAGE;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // We haven't defined the UI layout yet.
        //setContentView(R.layout.activity_main);

        if (loadNativeLibrary(getResources().getString(R.string.native_lib_name))) {
            File externalFilesDir = getExternalFilesDir("");
            if (externalFilesDir != null) {
                initFilePath(externalFilesDir.toString());
            }
        }

        requestPermissions(new String[]{ WRITE_EXTERNAL_STORAGE, READ_EXTERNAL_STORAGE}, 1);

        Intent intent = new Intent(MainActivity.this, NativeActivity.class);
        startActivity(intent);
        finishAffinity();
    }

    private boolean loadNativeLibrary(String nativeLibName) {
        boolean status = true;

        try {
            System.loadLibrary(nativeLibName);
        } catch (UnsatisfiedLinkError e) {
            Toast.makeText(getApplicationContext(), "Native code library failed to load.", Toast.LENGTH_SHORT).show();
            status = false;
        }

        return status;
    }

    private native void initFilePath(String external_dir);
}