FROM elandini84/r1images:tourCore2_cuda.11.7.1-cudnn8_iron_stable

#### Install python packages ####
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install --upgrade setuptools
RUN python3 -m pip install numpy opencv-contrib-python scikit-learn 

LABEL maintainer="Francesco Brand"
# Non-interactive installation mode
ENV DEBIAN_FRONTEND=noninteractive

ARG user_name="user1" 
ARG robot_code="/usr/local/src/robot"
USER user1
WORKDIR /home/user1/

### Install google from vcpkg ############################################# START ####
RUN sudo apt-get update && sudo apt-get install -y curl zip unzip tar
RUN mkdir Downloads
WORKDIR /home/${user_name}/Downloads
RUN git clone https://github.com/Microsoft/vcpkg.git && \
    cd vcpkg && git checkout ${google_cloud_version} && ./bootstrap-vcpkg.sh -disableMetrics && \
    ./vcpkg install --clean-after-build google-cloud-cpp[texttospeech] google-cloud-cpp[speech] google-cloud-cpp[dialogflow-cx]
ENV VCPKG_ROOT=/home/${user_name}/Downloads/vcpkg
### Packaging google-cloud-cpp instructions ############################################### END ####

ENV GOOGLE_APPLICATION_CREDENTIALS=/home/${user_name}/.config/google-credential/hsp_google.json
ENV BUILD_SUFFIX=build

WORKDIR ${robot_code}
RUN git clone https://github.com/robotology/yarp-devices-google-cloud && \
   cd yarp-devices-google-cloud && cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/${user_name}/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake && \
   cmake --build build

ENV YARP_DATA_DIRS=${YARP_DATA_DIRS}:${robot_code}/yarp-devices-google-cloud/build/share/yarp:${robot_code}/yarp-devices-google-cloud/build/share/

WORKDIR /home/${user_name}/Downloads
# # Install openai c++ dependencies
# RUN git clone https://github.com/Microsoft/vcpkg.git && \
#     cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics && \
#     ./vcpkg integrate install && ./vcpkg install curl[tool] && \
#     ./vcpkg install nlohmann-json

# Install openai c++ community library
RUN git clone https://github.com/D7EAD/liboai.git && \
    cd liboai && cd liboai && \
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/${user_name}/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_POSITION_INDEPENDENT_CODE=true && \
    cd build && make -j8 && sudo make install

# TODO: this should go into the entrypoint
# # Configure openai credentials
# RUN echo "\n" >> /home/${user_name}/.bashrc && \
#     echo "set -a" >> /home/${user_name}/.bashrc && \
#     echo "source ~/.env/config.env" >> /home/${user_name}/.bashrc && \
#     echo "set +a" >> /home/${user_name}/.bashrc

# Install yarp-devices-llm
ARG llm_branch

WORKDIR ${robot_code}
RUN git clone https://github.com/robotology/yarp-devices-llm -b ${llm_branch} && \
    cd yarp-devices-llm && mkdir build && \
    cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/home/${user_name}/Downloads/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DLIBOAI_INSTALL_PATH=/home/${user_name}/Downloads/liboai && cd build && \
    make -j8
ENV YARP_DEVICES_LLM_DIR=${robot_code}/yarp-devices-llm/build
ENV YARP_DATA_DIRS=${YARP_DATA_DIRS}:${YARP_DEVICES_LLM_DIR}/share/yarp:${YARP_DEVICES_LLM_DIR}/share
ENV PATH=$PATH:${YARP_DEVICES_LLM_DIR}/bin

ENV TOUR_GUIDE_ROBOT_SOURCE_DIR=${robot_code}/tour-guide-robot
ENV YARP_DATA_DIRS=${YARP_DATA_DIRS}:${TOUR_GUIDE_ROBOT_SOURCE_DIR}/build/share/tour-guide-robot
