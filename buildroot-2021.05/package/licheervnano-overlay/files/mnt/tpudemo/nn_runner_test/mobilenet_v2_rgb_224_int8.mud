[basic]
type = cvimodel
model = mobilenet_v2_rgb_224_int8.cvimodel

[extra]
model_type = classifier
input_type = rgb
mean = 123.675, 116.28, 103.53
scale = 0.017124753831663668, 0.01750700280112045, 0.017429193899782137
labels = imagenet_classes.txt

