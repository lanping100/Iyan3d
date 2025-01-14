package com.smackall.iyan3dPro.Helper;

public class SceneDB {
    int _id;
    String image;
    String sceneName;
    String time;

    public SceneDB() {
    }

    public SceneDB(int n, String string2, String string3, String string4) {
        this._id = n;
        this.sceneName = string2;
        this.image = string3;
        this.time = string4;
    }

    public SceneDB(String string2, String string3, String string4) {
        this.sceneName = string2;
        this.image = string3;
        this.time = string4;
    }

    public int getID() {
        return this._id;
    }

    public void setID(int n) {
        this._id = n;
    }

    public String getImage() {
        return this.image;
    }

    public void setImage(String string2) {
        this.image = string2;
    }

    public String getName() {
        return this.sceneName;
    }

    public void setName(String string2) {
        this.sceneName = string2;
    }

    public String getTime() {
        return this.time;
    }

    public void setTime(String string2) {
        this.time = string2;
    }
}

