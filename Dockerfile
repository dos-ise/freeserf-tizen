FROM ubuntu:22.04 AS base

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# Install required packages and dependencies
RUN apt-get update && apt-get install -y \
	cmake \
	expect \
	git \
	ninja-build \
	python3 \
	unzip \
	wget \
	xz-utils \
	default-jre \
	&& rm -rf /var/lib/apt/lists/*

# Create a non-root user and set up the working directory
RUN useradd -m -s /bin/bash freeserf
USER freeserf
WORKDIR /home/freeserf

# Install Tizen Studio CLI and configure the toolchain path
RUN wget -nv -O web-cli_Tizen_Studio_6.1_ubuntu-64.bin 'https://download.tizen.org/sdk/Installer/tizen-studio_6.1/web-cli_Tizen_Studio_6.1_ubuntu-64.bin'
RUN chmod a+x web-cli_Tizen_Studio_6.1_ubuntu-64.bin
RUN ./web-cli_Tizen_Studio_6.1_ubuntu-64.bin --accept-license --no-java-check /home/freeserf/tizen-studio
ENV PATH=/home/freeserf/tizen-studio/tools/ide/bin:/home/freeserf/tizen-studio/tools:${PATH}

# Prepare the Tizen certificate and security profiles for signing the application package
RUN tizen certificate \
	-a freeserf \
	-f freeserf \
	-p 1234
RUN tizen security-profiles add \
	-n freeserf \
	-a /home/freeserf/tizen-studio-data/keystore/author/freeserf.p12 \
	-p 1234

# Workaround to package applications without gnome-keyring
RUN sed -i 's|/home/freeserf/tizen-studio-data/keystore/author/freeserf.pwd||' /home/freeserf/tizen-studio-data/profile/profiles.xml
RUN sed -i 's|/home/freeserf/tizen-studio-data/tools/certificate-generator/certificates/distributor/tizen-distributor-signer.pwd|tizenpkcs12passfordsigner|' /home/freeserf/tizen-studio-data/profile/profiles.xml

# Install official Emscripten SDK
RUN git clone https://github.com/emscripten-core/emsdk.git /home/freeserf/emsdk
WORKDIR /home/freeserf/emsdk

RUN ./emsdk install latest
RUN ./emsdk activate latest
RUN echo 'source /home/freeserf/emsdk/emsdk_env.sh' >> /home/freeserf/.bashrc

# Copy the Freeserf source into the image
# NOTE: build this Dockerfile from the root of your local freeserf-tizen checkout
WORKDIR /home/freeserf
COPY --chown=freeserf . ./freeserf

# Build Freeserf using Emscripten (CMakeLists.txt lives at repo root)
WORKDIR /home/freeserf/freeserf
RUN git submodule update --init --recursive || true

# Copy CMake modules for Emscripten SDL2 support
RUN mkdir -p cmake && \
    if [ -f cmake-FindSDL2.cmake ]; then \
        cp cmake-FindSDL2.cmake cmake/FindSDL2.cmake; \
    fi && \
    if [ -f cmake-FindSDL2_mixer.cmake ]; then \
        cp cmake-FindSDL2_mixer.cmake cmake/FindSDL2_mixer.cmake; \
    fi && \
    if [ -f cmake-FindSDL2_image.cmake ]; then \
        cp cmake-FindSDL2_image.cmake cmake/FindSDL2_image.cmake; \
    fi

# Tizen 5.5 (2020 sets) runs Chromium 69, whose WebAssembly has no bulk memory, no
# non-trapping float-to-int and no BigInt at the JS/WASM boundary. Current Emscripten refuses
# MIN_CHROME_VERSION below 85, so these features are switched off directly. WASM_BIGINT=0
# prints a deprecation warning with recent Emscripten but still takes effect.
ENV WASM_COMPAT="-mno-bulk-memory -mno-bulk-memory-opt -mno-nontrapping-fptoint"

# Configure with CMake
RUN bash -lc "source /home/freeserf/emsdk/emsdk_env.sh && \
    emcmake cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/home/freeserf/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
        -DENABLE_TESTS=OFF \
        -DENABLE_SDL2_IMAGE=OFF \
        -DCMAKE_C_FLAGS='$WASM_COMPAT' \
        -DCMAKE_CXX_FLAGS='$WASM_COMPAT' \
        -DCMAKE_EXE_LINKER_FLAGS='-sWASM_BIGINT=0 $WASM_COMPAT' \
        -G Ninja \
        -S . \
        -B build"

# Build
RUN bash -lc "source /home/freeserf/emsdk/emsdk_env.sh && \
    emmake ninja -C build"

# The generated JS uses syntax newer than Chromium 69 (optional chaining, ??=, ||=).
# Lower it with esbuild, using the node that ships with emsdk.
RUN bash -lc "source /home/freeserf/emsdk/emsdk_env.sh && \
    mkdir -p /home/freeserf/lower && cd /home/freeserf/lower && \
    npm init -y >/dev/null && npm install --no-audit --no-fund esbuild@0.24.0 >/dev/null && \
    npx esbuild /home/freeserf/freeserf/build/FreeSerf.js \
        --target=chrome69 --allow-overwrite --log-level=warning \
        --outfile=/home/freeserf/freeserf/build/FreeSerf.js"

# Emscripten's prebuilt libc (memcpy/memset) still uses bulk memory, so lower the linked
# module and write it with only the features Chromium 69 accepts (sign-ext and mutable
# globals are supported there). wasm-opt fails the build if anything else is left.
RUN /home/freeserf/emsdk/upstream/bin/wasm-opt build/FreeSerf.wasm \
        --enable-bulk-memory --enable-bulk-memory-opt --enable-sign-ext \
        --enable-nontrapping-float-to-int --enable-mutable-globals \
        --llvm-memory-copy-fill-lowering --llvm-nontrapping-fptoint-lowering -O3 \
        -o build/FreeSerf.lowered.wasm && \
    /home/freeserf/emsdk/upstream/bin/wasm-opt build/FreeSerf.lowered.wasm \
        --mvp-features --enable-sign-ext --enable-mutable-globals \
        -o build/FreeSerf.wasm && \
    rm build/FreeSerf.lowered.wasm

# Assemble the Tizen widget directory
WORKDIR /home/freeserf
RUN mkdir -p widget/resources

# Emscripten's target has no .html suffix -> FreeSerf.js/.wasm
RUN cp freeserf/build/FreeSerf.js   widget/
RUN cp freeserf/build/FreeSerf.wasm widget/

# Fullscreen Tizen wrapper (no frame, no menus) + widget config + icon
RUN cp freeserf/tizen/index.html widget/index.html
RUN cp freeserf/res/config.xml widget/config.xml
RUN cp freeserf/cmake/icon.png widget/icon.png

# Runtime resources (TrueType font, UI images) loaded into the VFS by index.html
RUN cp freeserf/web/resources/settlers.ttf        widget/resources/
RUN cp freeserf/web/resources/button-advanced.png widget/resources/

# Sign and package the application into a WGT file
RUN echo \
	'set timeout -1\n' \
	'spawn tizen package -t wgt -- widget\n' \
	'expect "Author password:"\n' \
	'send -- "1234\\r"\n' \
	'expect "Yes: (Y), No: (N) ?"\n' \
	'send -- "N\\r"\n' \
	'expect eof\n' \
| expect

RUN mv widget/FreeSerf.wgt .

# Clean up unnecessary files to reduce image size
RUN rm -rf \
	widget \
	freeserf \
	web-cli_Tizen_Studio_6.1_ubuntu-64.bin \
	tizen-package-expect.sh \
	.package-manager \
	emsdk \
	.wget-hsts

# Use a multi-stage build to reclaim space from deleted files
FROM ubuntu:22.04
COPY --from=base /home/freeserf/FreeSerf.wgt /home/freeserf/FreeSerf.wgt
COPY --from=base /home/freeserf/tizen-studio /home/freeserf/tizen-studio
COPY --from=base /home/freeserf/tizen-studio-data /home/freeserf/tizen-studio-data

RUN useradd -m -s /bin/bash freeserf
RUN chown -R freeserf:freeserf /home/freeserf

USER freeserf
WORKDIR /home/freeserf

# Add Tizen Studio tools to PATH environment variable
ENV PATH=/home/freeserf/tizen-studio/tools/ide/bin:/home/freeserf/tizen-studio/tools:${PATH}
