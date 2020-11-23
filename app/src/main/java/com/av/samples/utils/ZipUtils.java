package com.av.samples.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class ZipUtils {

    private boolean checkZipDirValid(String zipDir) {
//        boolean hasBundleFile = false;
//        boolean hasJsonFile = false;
//        File dir = new File(zipDir);
//        if (dir.exists()) {
//            File files[] = dir.listFiles();
//            for (File files : files) {
//                if (fileName.endsWith(".json")) {
//                    hasJsonFile = true;
//                } else if (fileName.endsWith(".bundle")) {
//                    hasBundleFile = true;
//                }
//            }
//            return hasJsonFile && hasBundleFile;
//        }
        return false;
    }

    public boolean unzip(String zipFilePath, String destDir) {
        File zipFile = new File(zipFilePath);
        // 原始文件不存在
        if (!zipFile.exists()) {
            return false;
        }
        File dir = new File(destDir);
        // 之前已经解压，文件夹存在了就不用再解压了
        if (checkZipDirValid(destDir)) {
            return true;
        }
        if(!dir.exists()) {
            dir.mkdirs();
        }
        FileInputStream fis = null;
        byte[] buffer = new byte[1024];
        try {
            fis = new FileInputStream(zipFilePath);
            ZipInputStream zis = new ZipInputStream(fis);
            ZipEntry zipEntry = zis.getNextEntry();
            while (zipEntry != null) {
                String fileName = zipEntry.getName();
                File newFile = new File(destDir + File.separator + fileName);
                new File(newFile.getParent()).mkdirs();
                FileOutputStream fos = new FileOutputStream(newFile);
                int len;
                while ((len = zis.read(buffer)) > 0) {
                    fos.write(buffer, 0, len);
                }
                fos.close();
                //close this ZipEntry
                zis.closeEntry();
                zipEntry = zis.getNextEntry();
            }
            zis.closeEntry();
            zis.close();
            fis.close();
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        } finally {
            try {
                if (fis != null) {
                    fis.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return true;
    }


}
