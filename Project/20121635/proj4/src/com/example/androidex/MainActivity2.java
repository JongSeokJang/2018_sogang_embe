package com.example.androidex;


import java.util.Random;
import java.util.StringTokenizer;

import com.example.androidex.R;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Toast;



public class MainActivity2 extends Activity{
	
	native int myfunc_open();
	native int myfunc_write(int fd, int flag);
	native int myfunc_close(int fd);

	
	private class Timer_Exit extends Thread{
		
		public void run(){
			
			while( myfunc_write(fd,100)!= 0 ){
				try{
					Thread.sleep(1000);
				} catch (InterruptedException e){
					//break;
				}
			}
			finish();
		}
	}
	
	LinearLayout 	linear2;
	EditText 	fieldInput;
	
//	int black_color = 0xff000000;	
	int fd;
	int row;
	int col;
	int black_btnId;
	
	public int check_complete(){
		
		for (int i = 0; i < row; i++){
			for(int j = 0; j < col; j++){
				
				int btnId = i*col + (j+1);
				
				Button tempBtn = (Button)findViewById(btnId);
				int  btnText = Integer.parseInt(tempBtn.getText().toString());
				
				if( btnText == btnId )
					continue;
				else
					return 0;	
			}
		}
		System.out.println("In check_complete : 1");
		return 1;
		
	}
	
	public void swap_puzzle(int black_btn, int change_btn, int mode){
		
		if( mode == 1 )
			myfunc_write(fd,200);
		
		Button blackBtn = (Button)findViewById(black_btn);
		Button changeBtn = (Button)findViewById(change_btn);
		
		String blackText = blackBtn.getText().toString();
		String changeText = changeBtn.getText().toString();
		
		changeBtn.setBackgroundColor(Color.BLACK);
		blackBtn.setBackgroundResource(android.R.drawable.btn_default);
				
		blackBtn.setText(changeText);
		changeBtn.setText(blackText);
		
	}
	
	public void suffle_Puzzle(){
		
		int shuffle_count = 400;
		
		int black_btn_col = col;
		int black_btn_row = row;
		int change_btnId = 0;
		for( int i = 0; i < shuffle_count; i++){
			
			Random rand = new Random();
			int changeSide = (rand.nextInt(100000))%4;
			
			black_btnId = (black_btn_row - 1)*col + black_btn_col; 
									
			switch (changeSide){
				
				case 0: // up side
					
					if( black_btn_row > 1 ){
						black_btn_row -= 1;
						change_btnId = black_btnId - col; 
						swap_puzzle(black_btnId, change_btnId, 0);
					}					
					break;
					
					
				case 1:	// right side
					
					if( black_btn_col < col ){
						black_btn_col += 1;
						change_btnId = black_btnId + 1; 
						swap_puzzle(black_btnId, change_btnId, 0);
					}
					break;
					
				case 2:	// left side
					
					if( black_btn_col > 1 ){
						black_btn_col -= 1;
						change_btnId = black_btnId - 1;
						swap_puzzle(black_btnId, change_btnId, 0);
					}
					break;
					
				case 3:	// down side
					
					if( black_btn_row < row ){
						black_btn_row += 1;
						change_btnId = black_btnId + col;
						swap_puzzle(black_btnId, change_btnId, 0);
					}
					break;
				default:
			}
			
		}
		black_btnId = (black_btn_row - 1)*col + black_btn_col;
		
	}
	
	public void make_Puzzle(){
				
		
		LinearLayout.LayoutParams p = new LinearLayout.LayoutParams
										(LinearLayout.LayoutParams.MATCH_PARENT,
										 LinearLayout.LayoutParams.MATCH_PARENT);
		p.weight = 1;
		
		for( int i = 0; i < row; i++){

			LinearLayout puzzle_row = new LinearLayout(getApplicationContext());
			puzzle_row.setLayoutParams(p);

			for( int j  = 0; j < col; j++){
				
				int btnId = i*col + (j+1); 
				
				Button PuzzleBtn = new Button(getApplicationContext());
				
				OnClickListener listener = new OnClickListener(){
					public void onClick(View v){
						
						
						int tempBtnId = v.getId();
						System.out.println("btnID : "+ tempBtnId + "black : " + black_btnId);
						
						if( tempBtnId-col == black_btnId ){ // compare up
							swap_puzzle( black_btnId,tempBtnId,1);
							black_btnId = tempBtnId; 
						}
						else if (tempBtnId + 1 == black_btnId && (tempBtnId % col) != 0  ){ // compare right
							swap_puzzle(black_btnId,tempBtnId, 1);
							black_btnId = tempBtnId;
						}
						else if (tempBtnId + col == black_btnId ){ // compare down
							swap_puzzle(black_btnId,tempBtnId, 1);
							black_btnId = tempBtnId;
						}
						else if (tempBtnId - 1 == black_btnId && (tempBtnId % col) != 1 ){  // compare left
							swap_puzzle(black_btnId,tempBtnId, 1);
							black_btnId = tempBtnId; 
						}
						
						
						if( check_complete() == 1 )
							finish();
					}
				};					
								
				PuzzleBtn.setId( btnId );
				PuzzleBtn.setLayoutParams(p);
				PuzzleBtn.setText( String.valueOf(btnId) );
				PuzzleBtn.setOnClickListener(listener);
				
				System.out.println("btnID"+ btnId +"col:"+ col +"row:"+ row);
				
				if( btnId == col*row )
					PuzzleBtn.setBackgroundColor(Color.BLACK);
			
				puzzle_row.addView(PuzzleBtn);
			}
			linear2.addView(puzzle_row);
		}	
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		System.loadLibrary("mylib");
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		fd = myfunc_open();
		
		Timer_Exit t = new Timer_Exit();
		t.start();
		Button makeBtn 	= (Button)findViewById(R.id.puzzle_make_button);//find make buttons id	
		fieldInput		= (EditText)findViewById(R.id.puzzle_size_input);//find edit text id
		
		linear2 = (LinearLayout)findViewById(R.id.puzzle_container);
		linear2.setOrientation(LinearLayout.VERTICAL);
		
		OnClickListener listener = new OnClickListener(){
			
			@Override
			public void onClick(View v) {
				
				try{
					
					String firstInput = fieldInput.getText().toString();
					fieldInput.setText(""); 
					StringTokenizer string_token = new StringTokenizer(firstInput);
					
					row = Integer.parseInt(string_token.nextElement().toString());
					col = Integer.parseInt(string_token.nextElement().toString());
							
					if( row > 4 || col > 4 ){
						Toast.makeText(getApplicationContext(), "Puzzle size over 4", Toast.LENGTH_SHORT).show();
						finish();
					}
					if( row < 2 || col < 2 ){
						Toast.makeText(getApplicationContext(), "Puzzle size under 2", Toast.LENGTH_SHORT).show();
						finish();
					}
					
					make_Puzzle();
					suffle_Puzzle();					
				}
				catch(Exception e){
					Toast.makeText(getApplicationContext(), "Please check Input", Toast.LENGTH_SHORT).show();
					finish();		
				}
			}
		};
		makeBtn.setOnClickListener(listener);
	}
	
	@Override
	protected void onDestroy() {
		super.onDestroy();
		myfunc_close(fd);
	}
		
	

}
