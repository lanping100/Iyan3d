package com.smackall.animator.Adapters;

import android.app.Activity;
import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.smackall.animator.EditorView;
import com.smackall.animator.Helper.Constants;
import com.smackall.animator.Helper.FileHelper;
import com.smackall.animator.Helper.PathManager;
import com.smackall.animator.R;

import java.io.File;
import java.io.FilenameFilter;

/**
 * Created by Sabish.M on 18/3/16.
 * Copyright (c) 2015 Smackall Games Pvt Ltd. All rights reserved.
 */
public class ChangeTextureAdapter extends BaseAdapter {

    private Context mContext;
    private GridView gridView;
    public File[] files;

    public ChangeTextureAdapter(Context c,GridView gridView) {
        mContext = c;
        this.gridView = gridView;
    }

    @Override
    public int getCount() {
        this.files = getFileList();
        return ((this.files != null && this.files.length > 0) ? files.length : 0) +  1;
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        final View grid;
        if(convertView==null){
            LayoutInflater inflater=((Activity)mContext).getLayoutInflater();
            grid=inflater.inflate(R.layout.asset_cell, parent, false);

        }else{
            grid = (View)convertView;
        }

        grid.getLayoutParams().height = this.gridView.getHeight()/5;
        ((ProgressBar)grid.findViewById(R.id.progress_bar)).setVisibility(View.INVISIBLE);
        ((ImageView)grid.findViewById(R.id.thumbnail)).setVisibility(View.VISIBLE);

            if(position == 0){
                ((ImageView) grid.findViewById(R.id.thumbnail)).setBackgroundColor(Color.argb(255,(int)((EditorView)((Activity)mContext)).textureSelection.assetsDB.getX()*255
                ,(int)((EditorView)((Activity)mContext)).textureSelection.assetsDB.getY()*255,(int)((EditorView)((Activity)mContext)).textureSelection.assetsDB.getZ()*255));
                ((TextView)grid.findViewById(R.id.assetLable)).setText("Pick Color");
            }
            else if(files[position-1].exists()) {
                ((ImageView) grid.findViewById(R.id.thumbnail)).setImageBitmap(BitmapFactory.decodeFile(PathManager.LocalThumbnailFolder+"/"+FileHelper.getFileNameFromPath(files[position-1].toString())));
                ((TextView)grid.findViewById(R.id.assetLable)).setText(FileHelper.getFileWithoutExt(FileHelper.getFileNameFromPath(files[position-1].toString())));
            }

        grid.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(position == 0)
                    ((EditorView)((Activity)mContext)).colorPicker.showColorPicker(gridView.getChildAt(0),null,Constants.CHANGE_TEXTURE_MODE);
                else {
                    ((EditorView) ((Activity) mContext)).textureSelection.assetsDB.setTexture(FileHelper.getFileWithoutExt(FileHelper.getFileNameFromPath(files[position - 1].toString())));
                    ((EditorView) ((Activity) mContext)).textureSelection.assetsDB.setIsTempNode(true);
                    ((EditorView) ((Activity) mContext)).textureSelection.changeTexture();
                }
            }
        });

        return grid;
    }

    private File[] getFileList()
    {
        final String userFolder = PathManager.LocalImportedImageFolder+"/";
        final File f = new File(userFolder);
        FilenameFilter filenameFilter = new FilenameFilter() {
            @Override                                                                                                                               
            public boolean accept(File dir, String filename) {
                if(filename.toLowerCase().endsWith("png"))
                    return true;
                else
                    return false;
            }
        };
        File files[] = f.listFiles(filenameFilter);
        return files;
    }
}
