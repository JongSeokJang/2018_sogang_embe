package com.example.androidex;

import android.app.Activity;
import android.os.Bundle;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;



public class MainActivity extends Activity{
	
	LinearLayout linear;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		linear = (LinearLayout)findViewById(R.id.container);
		linear.setOrientation(LinearLayout.VERTICAL);
		
		for(int i = 0; i < 3; i++){
			LinearLayout row = new LinearLayout(this);
	
			row.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
			
			for(int j = 0; j <4; j++){
				Button btnTag = new Button(this);
				btnTag.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT))
				;
				btnTag.setText("Button" + (j+i+(i*4)));
				row.addView(btnTag);
				
			}
			linear.addView(row);
		}
	}

}
