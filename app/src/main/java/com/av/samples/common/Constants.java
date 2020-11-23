package com.av.samples.common;

public class Constants {



    public enum FlushMode {
        OFF(0),
        ON(1),
        AUTO(2);

        public final int id;
        FlushMode(int id){
            this.id = id;
        }
    }
}
