package hermes.Views;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.oplabs.hermes.R;

import general.HermesActivity;

public class MainActivity extends HermesActivity {

    private final String TAG = "HermesMainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.i(TAG, "OnCreate");

    }

    //Methods for changing views
    //Run test
    public void runTest(View view) {
        Log.i(TAG, "RunTest");
        Intent myIntent = new Intent(MainActivity.this, ResultsActivity.class);
        MainActivity.this.startActivity(myIntent);
    }

    //Activity Methods
    public void goToLogin(View view) {
        //Temporary until we get a login page
        goToSettings(view);
    }

    public void goToSettings(View view) {
        Intent intent = new Intent(this, SettingsActivity.class);
        intent.putExtra(SettingsActivity.EXTRA_SHOW_FRAGMENT, SettingsActivity.GeneralPrefs.class.getName());
        intent.putExtra(SettingsActivity.EXTRA_NO_HEADERS, true);
        startActivity(intent);
    }
}