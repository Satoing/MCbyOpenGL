import tinify#导入tinify插件
import sys
from os import path
import os
import os.path
  
# Please reset the root directory Path !  
ImageFilePath = "."#要压缩的图片的目录
tinify.key = "l7SZ29QJFVz2vstt5cZqcPRhzBwDZjx2"#在以上步骤中获取到的API key
# 这个额度用光了可以用这个：kDWNj3b4kyNJyddzK9GkjhwcTl95bWK2

#获取到本目录中的所有文件
def getFilesAbsolutelyPath(ImageFilePath):  
    currentfiles = os.listdir(ImageFilePath)  
    filesVector = []  
    for file_name in currentfiles:  
        fullPath = os.path.join(ImageFilePath, file_name)  
        if os.path.isdir(fullPath):  
            newfiles = getFilesAbsolutelyPath(fullPath)
            filesVector.extend(newfiles)  
        else:  
            filesVector.append(fullPath)  
    return filesVector
  
filePathVector = getFilesAbsolutelyPath(ImageFilePath)  
pngFile = []  
  
#筛选出所有png文件（现在看来这一步可以省略了，直接把这些判定写在上一个函数中，filePathVector就直接筛选出png文件了）
for filename in filePathVector:  
    flag = filename.find(".png")
    metaNot = filename.find(".meta")
    if flag != -1 and metaNot == -1:
        pngFile.append(filename)

#对每个png文件进行上传压缩，并下载，上传和下载的过程调用插件即可完成
for filename in pngFile:
    print(filename + ' start')
    source = tinify.from_file(filename)
    source.to_file(filename)
    print(filename + ' ok')