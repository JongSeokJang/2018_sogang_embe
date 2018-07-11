package com.example.androidex;

import com.example.androidex.R;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;



public class MainActivity extends Activity {
	Button btn1;
	LinearLayout linear;

	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);//show activity_main.xml layout
		btn1=(Button)findViewById(R.id.btnpuzzle);//if you push this button, puzzle game will be start.
		
		OnClickListener listener = new OnClickListener(){

			@Override
			public void onClick(View arg0) {
				
				// TODO Auto-generated method stub
				Intent intent = new Intent(MainActivity.this,MainActivity2.class);
				startActivity(intent);//call PuzzleActivity class
			}
			
		};
		btn1.setOnClickListener(listener);//set listener
	}
	
}

