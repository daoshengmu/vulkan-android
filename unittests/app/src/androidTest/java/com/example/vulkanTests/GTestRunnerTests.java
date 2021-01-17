package com.example.vulkanTests;

import android.util.Log;
import androidx.test.core.app.ApplicationProvider;
import org.apache.commons.lang3.mutable.MutableObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.List;

import static org.junit.Assert.assertTrue;

@RunWith(Parameterized.class)
public class GTestRunnerTests {
    public String TAG= getClass().getSimpleName();

    @Parameterized.Parameters(name = "{0}")
    public static List<String> getTestList() {
        GTestRunner gtestRunner= new GTestRunner(ApplicationProvider.getApplicationContext());
        return gtestRunner.getTests();
    }

    protected String testName;
    protected GTestRunner gtestRunner;

    public GTestRunnerTests(String testName) {
        this.testName= testName;
        gtestRunner= new GTestRunner(ApplicationProvider.getApplicationContext());
    }

    @Test
    public void testGTest() {
        MutableObject<String> output= new MutableObject<>();
        boolean success = gtestRunner.run("--gtest_filter=" + testName, output);
        Log.d(TAG, output.toString());
        assertTrue(success);
    }
}
