CUDA_VER=12.8
 
 
APP:= deepstream_app
PKGS:= glib-2.0 gobject-2.0 json-glib-1.0 uuid gstreamer-1.0   
SRCS:= $(wildcard *.cpp) $(wildcard utils/*.cpp) $(wildcard pipeline/*.cpp) $(wildcard src/*.cpp)

# Include paths
CFLAGS+= -I/opt/nvidia/deepstream/deepstream/sources/includes \
		-I/usr/local/cuda-$(CUDA_VER)/include 

# Package config flags
CFLAGS+= $(shell pkg-config --cflags $(PKGS))

# Package config libs
LIBS:= $(shell pkg-config --libs $(PKGS))

# DeepStream and CUDA libs with correct paths
# ADDED -lnvds_obj_encode below to fix your linker error
LIBS+= -L/opt/nvidia/deepstream/deepstream/lib \
       -L/usr/local/cuda-$(CUDA_VER)/lib64 \
       -lnvdsgst_helper -lnvdsgst_meta -lnvds_meta \
       -lgstrtspserver-1.0 \
       -lnvbufsurface -lnvbufsurftransform \
       -lnvds_yml_parser -lnvds_batch_jpegenc \
       -lcudart -lcuda -lm 

# Add rpath to find libraries at runtime
LIBS+= -Wl,-rpath,/opt/nvidia/deepstream/deepstream/lib

$(APP): $(SRCS)
	$(CXX) -o $(APP) $(SRCS) $(CFLAGS) $(LIBS)

clean:
	rm -rf $(APP)
