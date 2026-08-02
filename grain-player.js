(() => {
  "use strict";

  const SAMPLE_RATE = 16000;
  const FIRMWARE = Object.freeze({
    speed: 0.2,
    pitchCenterMm: 200,
    pitchSemisPer3x: 19,
    pitchBaseSemis: 12,
    pitchTopSemis: 36,
    ringScale: 64
  });

  // Measured grain 1 from the firmware's UDP payload gallery.
  const SOURCE_GRAIN = new Int16Array([
  0,-2,-8,-13,-3,2,-21,-31,-37,-41,-46,-51,-54,-59,-63,-67,-71,-75,-78,-83,-87,-90,-95,-97,
  -102,-104,-109,-113,-115,-119,-122,-126,-130,-45,144,192,207,213,219,222,228,503,600,652,672,685,697,706,
  718,1010,1127,1201,1257,1281,1302,1316,1333,1121,1231,1563,2184,2526,2626,2668,2694,1443,-184,-934,-1587,-1728,-1752,-1748,
  -1736,2722,5678,8276,10313,11709,11961,11957,11897,4162,-140,-3268,-5052,-6083,-6472,-6524,-6504,-5517,-4278,-2574,-1042,248,537,601,
  2101,5393,7322,8962,10265,11989,12322,12338,12278,13269,12138,10021,7763,5858,5096,4904,4836,9122,12563,16497,21000,23454,23879,23855,
  23735,6023,-2145,-6921,-9555,-10470,-10622,-10606,-10550,-8509,-7037,-4679,-2931,-922,184,429,481,7655,11801,15739,19316,22560,23835,24000,
  23919,22079,17836,14319,10009,6516,5710,5505,5433,-665,-2891,-3861,-4623,-5088,-5277,-5293,-5269,-6476,-7266,-7566,-7410,-7125,-6961,-6893,
  -6841,-3360,-561,-116,-477,-906,-998,-1014,-1010,-2919,-3143,-2939,-1892,-160,224,308,324,2839,2887,866,-557,-1664,-2113,-2205,
  -2213,-2105,-2381,-3208,-4366,-4603,-4635,-4619,-4591,-2666,-2634,-3107,-4194,-4415,-4443,-4427,-4403,3500,8364,11428,12727,12904,12595,12467,
  12374,9660,7791,5289,2450,-573,-2089,-2410,-2470,-5890,-6452,-6267,-4711,-3352,-3031,-2947,-2911,-1178,-1014,-1712,-2357,-2991,-3360,-3424,
  -3420,-3901,-3985,-3985,-3965,-3941,-3917,-3893,-3869,-2855,-2614,-2550,-2522,-2506,-2490,-2474,-2458,-2879,-3015,-3340,-3584,-3292,-3043,-2975,
  -2943,-1158,-236,128,24,-208,-328,-352,-356,-2041,-2702,-2253,-1736,-1114,-970,-934,-922,3532,5497,6404,7069,6688,6572,6512,
  6468,2875,1383,425,-942,-2305,-2955,-3083,-3095,-4707,-5309,-5505,-5521,-5501,-5469,-5433,-5401,-5369,-5333,-5301,-5265,-5233,-5201,-5168,
  -5136,-3989,-3468,-2947,-2321,-1664,-1291,-1203,-1178,-204,40,92,104,104,104,104,104,733,1014,1198,1323,1343,1343,1335,
  1327,605,256,-40,-372,-545,-577,-585,-581,-577,-573,-573,-569,-565,-561,-557,-553,-1046,-1150,-1166,-1166,-1158,-1150,-1146,
  -1138,-2013,-2337,-2662,-2919,-2963,-2959,-2943,-2927,-3853,-4086,-4118,-4102,-4082,-4054,-4030,-4006,-3071,-2586,-2153,-2045,-2013,-1992,-1980,
  -1968,24,709,1106,1467,1760,1816,1816,1808,1539,1371,1235,1022,701,533,493,485,132,-40,-236,-505,-725,-769,-777,
  -773,-1511,-1916,-2177,-2410,-2578,-2602,-2598,-2582,-2574,-2438,-2269,-2101,-1960,-1876,-1852,-1836,-1824,-1812,-1800,-1788,-1776,-1768,-1756,
  -1744,-1311,-1174,-1098,-802,-513,-445,-429,-425,886,1567,2233,2618,2694,2694,2682,2666,2201,1800,1327,966,485,152,76,
  60,-1427,-1932,-2217,-2397,-2426,-2422,-2410,-2393,-2875,-2967,-2975,-2959,-2943,-2923,-2907,-2887,-2506,-2361,-2277,-2217,-2181,-2161,-2145,
  -2133,-1255,-441,240,986,1543,1660,1676,1672,3151,3356,3167,2939,2875,2843,2329,2201,793,44,-581,-1198,-1555,-1624,-1632,
  -1628,-2582,-2742,-2450,-2257,-2201,-2181,-2165,-2149,-1363,-1054,-890,-850,-838,-830,-826,-822,128,380,437,445,445,445,441,
  437,132,12,-64,-124,-168,-188,-192,-192,-689,-926,-1118,-1283,-1403,-1423,-1423,-918,-934,-802,-1267,-1363,-1375,-1371,-1367,
  -1355,-1347,-1339,-1331,-1323,-1315,-1307,-1299,-1291,-292,108,380,569,609,613,613,609,605,601,597,593,589,585,585,
  581,80,-28,-52,-56,-56,-56,-56,-56,-541,-657,-677,-681,-677,-673,-669,-665,-1154,-1259,-1275,-1271,-1267,-1259,-1251,
  -1243,-830,-697,-633,-593,-581,-577,-573,-569,76,320,473,589,669,685,685,681,348,172,68,44,40,36,36,
  36,-585,-866,-1050,-1166,-990,-661,-585,-565,-312,-188,-96,-16,40,72,80,80,80,80,80,76,76,76,76,
  76,-417,-525,-545,-549,-545,-541,-537,-533,-533,-529,-525,-521,-517,-517,-513,-509,136,380,533,645,729,745,745,
  741,657,713,228,120,96,88,88,84,-372,-501,-288,-196,-465,-521,-529,-529,-525,-525,-521,-517,-513,-509,-505,
  -505,-56,68,116,128,132,128,128,128,-445,-709,-894,-1038,-1102,-1110,-1110,-1102,-1535,-1672,-1491,-1158,-1078,-1058,-1046,
  -1038,-633,-485,-409,-388,-384,-380,-376,-376,449,793,1026,1243,1415,1483,1491,1483,1860,2017,2069,2073,2061,2049,2037,
  2025,1620,1459,1375,1351,1339,1331,1323,818,208,-172,-505,-797,-1066,-1186,-1207,-1207,-1836,-2249,-2402,-2426,-2418,-2406,-2389,
  -2373,-2061,-1916,-1820,-1744,-1688,-1668,-1656,-1644,-994,-697,-497,-376,-348,-340,-336,-336,609,862,910,918,914,910,906,
  898,1391,1491,1507,1503,1495,1483,1475,1467,1022,886,822,806,797,793,785,781,36,-244,-405,-469,-481,-481,-481,
  -477,-970,-1074,-1090,-1090,-1086,-1078,-1070,-1066,-561,-946,-914,-685,-465,-413,-396,-392,-886,-625,-673,-729,-449,-388,-372,
  -364,-360,-360,-356,-356,-352,-352,-348,-348,-344,-340,-340,-336,-336,-332,-332,-328,-328,168,28,-248,184,280,300,
  300,300,300,296,296,292,292,288,288,284,284,284,280,280,276,276,272,272,272,268,268,264,264,260,
  260,-228,-340,-364,-368,-364,-364,-360,-360,-797,-930,-970,-974,-970,-966,-958,-954,-569,-421,-344,-312,-797,-902,-425,
  -316,-24,120,228,304,336,340,340,336,336,332,332,328,328,324,324,320,320,316,316,312,312,308,308,
  300,-112,-217,-251,-269,-279,-282,-280,-272,-240,-252,-250,-246,-241,-233,-228,-220,-216,-208,-204,-196,-192,-187,-180,
  -175,-169,-164,-157,-153,-146,-142,-138,-132,-127,-121,-117,-111,-107,-103,-97,-93,74,107,112,109,103,96,91,
  84,78,72,66,60,54,49,43,37,-8,-16,-16,-13,-8,-4,0
  ]);

  const clamp = (value, minimum, maximum) => Math.min(maximum, Math.max(minimum, value));
  const peakUnits = SOURCE_GRAIN.reduce((peak, value) => Math.max(peak, Math.abs(value)), 1);
  const peakMm = Math.max(1, peakUnits / FIRMWARE.ringScale);
  const pitchSemis = clamp(
    FIRMWARE.pitchBaseSemis -
      FIRMWARE.pitchSemisPer3x * (Math.log(peakMm / FIRMWARE.pitchCenterMm) / Math.log(3)),
    FIRMWARE.pitchBaseSemis,
    FIRMWARE.pitchTopSemis
  );

  function cubicSample(data, position) {
    const index = Math.floor(position);
    if (index < 1 || index + 2 >= data.length) {
      if (index < 0 || index + 1 >= data.length) return 0;
      const fraction = position - index;
      return data[index] * (1 - fraction) + data[index + 1] * fraction;
    }
    const fraction = position - index;
    const p0 = data[index - 1];
    const p1 = data[index];
    const p2 = data[index + 1];
    const p3 = data[index + 2];
    return 0.5 * (
      (2 * p1) + (-p0 + p2) * fraction +
      (2 * p0 - 5 * p1 + 4 * p2 - p3) * fraction * fraction +
      (-p0 + 3 * p1 - 3 * p2 + p3) * fraction * fraction * fraction
    );
  }

  function fft(real, imaginary, inverse) {
    const count = real.length;
    for (let i = 1, j = 0; i < count; i += 1) {
      let bit = count >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) {
        let temporary = real[i];
        real[i] = real[j];
        real[j] = temporary;
        temporary = imaginary[i];
        imaginary[i] = imaginary[j];
        imaginary[j] = temporary;
      }
    }
    for (let length = 2; length <= count; length <<= 1) {
      const angle = (inverse ? 2 : -2) * Math.PI / length;
      const stepReal = Math.cos(angle);
      const stepImaginary = Math.sin(angle);
      for (let offset = 0; offset < count; offset += length) {
        let currentReal = 1;
        let currentImaginary = 0;
        for (let k = 0; k < length / 2; k += 1) {
          const upperReal = real[offset + k];
          const upperImaginary = imaginary[offset + k];
          const lowerReal = real[offset + k + length / 2] * currentReal -
            imaginary[offset + k + length / 2] * currentImaginary;
          const lowerImaginary = real[offset + k + length / 2] * currentImaginary +
            imaginary[offset + k + length / 2] * currentReal;
          real[offset + k] = upperReal + lowerReal;
          imaginary[offset + k] = upperImaginary + lowerImaginary;
          real[offset + k + length / 2] = upperReal - lowerReal;
          imaginary[offset + k + length / 2] = upperImaginary - lowerImaginary;
          const nextReal = currentReal * stepReal - currentImaginary * stepImaginary;
          currentImaginary = currentReal * stepImaginary + currentImaginary * stepReal;
          currentReal = nextReal;
        }
      }
    }
    if (inverse) {
      for (let i = 0; i < count; i += 1) {
        real[i] /= count;
        imaginary[i] /= count;
      }
    }
  }

  function phaseVocoder(input, pitchRatio, targetLength) {
    const frameSize = 1024;
    const nominalHop = 256;
    const analysisHop = Math.max(16, Math.round(nominalHop / pitchRatio));
    const synthesisHop = Math.max(1, Math.round(analysisHop * pitchRatio));
    const frameCount = Math.max(1, Math.ceil(input.length / analysisHop));
    const fullLength = (frameCount - 1) * synthesisHop + frameSize;
    const output = new Float32Array(fullLength);
    const normalization = new Float32Array(fullLength);
    const windowValues = new Float32Array(frameSize);
    const previousPhase = new Float32Array(frameSize / 2 + 1);
    const accumulatedPhase = new Float32Array(frameSize / 2 + 1);
    const real = new Float32Array(frameSize);
    const imaginary = new Float32Array(frameSize);

    for (let i = 0; i < frameSize; i += 1) {
      windowValues[i] = 0.5 - 0.5 * Math.cos(2 * Math.PI * i / frameSize);
    }

    for (let frame = 0; frame < frameCount; frame += 1) {
      const inputPosition = frame * analysisHop;
      const outputPosition = frame * synthesisHop;
      for (let i = 0; i < frameSize; i += 1) {
        real[i] = (inputPosition + i < input.length ? input[inputPosition + i] : 0) * windowValues[i];
        imaginary[i] = 0;
      }
      fft(real, imaginary, false);
      for (let bin = 0; bin <= frameSize / 2; bin += 1) {
        const magnitude = Math.hypot(real[bin], imaginary[bin]);
        const phase = Math.atan2(imaginary[bin], real[bin]);
        const expected = 2 * Math.PI * analysisHop * bin / frameSize;
        let difference = phase - previousPhase[bin] - expected;
        difference -= 2 * Math.PI * Math.round(difference / (2 * Math.PI));
        const trueFrequency = 2 * Math.PI * bin / frameSize + difference / analysisHop;
        previousPhase[bin] = phase;
        accumulatedPhase[bin] = frame === 0
          ? phase
          : accumulatedPhase[bin] + synthesisHop * trueFrequency;
        real[bin] = magnitude * Math.cos(accumulatedPhase[bin]);
        imaginary[bin] = magnitude * Math.sin(accumulatedPhase[bin]);
        if (bin > 0 && bin < frameSize / 2) {
          real[frameSize - bin] = real[bin];
          imaginary[frameSize - bin] = -imaginary[bin];
        }
      }
      fft(real, imaginary, true);
      for (let i = 0; i < frameSize && outputPosition + i < fullLength; i += 1) {
        output[outputPosition + i] += real[i] * windowValues[i];
        normalization[outputPosition + i] += windowValues[i] * windowValues[i];
      }
    }

    const renderedLength = Math.min(fullLength, targetLength);
    const rendered = new Float32Array(renderedLength);
    for (let i = 0; i < renderedLength; i += 1) {
      rendered[i] = output[i] / Math.max(normalization[i], 0.5);
    }
    return rendered;
  }

  function renderFirmwareGrain(data) {
    const pitchRatio = Math.pow(2, pitchSemis / 12);
    const sourceStep = FIRMWARE.speed * pitchRatio;
    const resampledLength = Math.floor(data.length / sourceStep);
    const normalized = Math.min(100, 32000 / peakUnits);
    const resampled = new Float32Array(resampledLength);
    for (let i = 0; i < resampledLength; i += 1) {
      resampled[i] = cubicSample(data, i * sourceStep) * normalized / 32768;
    }
    const targetLength = Math.floor(data.length / FIRMWARE.speed);
    const rendered = phaseVocoder(resampled, pitchRatio, targetLength);
    const fadeIn = Math.min(160, rendered.length >> 2);
    const fadeOut = Math.min(480, rendered.length >> 2);
    for (let i = 0; i < fadeIn; i += 1) rendered[i] *= i / fadeIn;
    for (let i = 0; i < fadeOut; i += 1) rendered[rendered.length - 1 - i] *= i / fadeOut;
    return rendered;
  }

  const renderedGrain = renderFirmwareGrain(SOURCE_GRAIN);
  const playButton = document.getElementById("grain-play");
  const canvas = document.getElementById("grain-wave");
  const renderMeta = document.getElementById("grain-render-meta");
  const controls = {
    volume: document.getElementById("audio-volume"),
    time: document.getElementById("delay-time"),
    feedback: document.getElementById("delay-feedback"),
    mix: document.getElementById("delay-mix")
  };
  const outputs = {
    volume: document.getElementById("volume-value"),
    time: document.getElementById("delay-time-value"),
    feedback: document.getElementById("delay-feedback-value"),
    mix: document.getElementById("delay-mix-value")
  };

  if (!playButton || !canvas || Object.values(controls).some((control) => !control)) return;

  let audioContext = null;
  let activeGraph = null;
  let stopTimer = 0;

  function values() {
    const normalizedTime = Number(controls.time.value);
    return {
      volume: Number(controls.volume.value),
      normalizedTime,
      delaySeconds: (40 + 660 * normalizedTime) / 1000,
      feedback: Number(controls.feedback.value),
      mix: Number(controls.mix.value)
    };
  }

  function refreshLabels() {
    const current = values();
    outputs.volume.textContent = Math.round(current.volume * 100) + "%";
    outputs.time.textContent = Math.round(current.delaySeconds * 1000) + " ms";
    outputs.feedback.textContent = Math.round(current.feedback * 100) + "%";
    outputs.mix.textContent = Math.round(current.mix * 100) + "%";
    renderMeta.textContent =
      "FIRMWARE RENDER / SPEED " + FIRMWARE.speed.toFixed(1) +
      "× / PITCH +" + pitchSemis.toFixed(1) + " ST";
  }

  function applyLiveParameters() {
    refreshLabels();
    if (!activeGraph || !audioContext) return;
    const current = values();
    const now = audioContext.currentTime;
    activeGraph.master.gain.setTargetAtTime(current.volume, now, 0.015);
    activeGraph.delay.delayTime.setTargetAtTime(current.delaySeconds, now, 0.015);
    activeGraph.feedback.gain.setTargetAtTime(Math.sqrt(current.feedback) * 0.96, now, 0.015);
    activeGraph.wet.gain.setTargetAtTime(current.mix, now, 0.015);
    schedulePlaybackStop();
  }

  function schedulePlaybackStop() {
    window.clearTimeout(stopTimer);
    stopTimer = 0;
    if (!activeGraph || !audioContext) return;
    const current = values();
    const feedbackGain = Math.sqrt(current.feedback) * 0.96;
    let repeats = current.mix > 0 ? 1 : 0;
    if (repeats && feedbackGain > 0.004 && feedbackGain < 1) {
      repeats = Math.max(1, Math.ceil(Math.log(0.004 / Math.max(current.mix, 0.004)) / Math.log(feedbackGain)));
    }
    const dryDuration = renderedGrain.length / SAMPLE_RATE;
    const tailDuration = Math.min(20, current.delaySeconds * repeats);
    const elapsed = audioContext.currentTime - activeGraph.startedAt;
    const remaining = Math.max(0.08, dryDuration + tailDuration + 0.12 - elapsed);
    stopTimer = window.setTimeout(() => stopPlayback(false), remaining * 1000);
  }

  function setPlayingState(playing) {
    playButton.classList.toggle("is-playing", playing);
    playButton.setAttribute("aria-pressed", String(playing));
    playButton.textContent = playing
      ? playButton.dataset.labelStop || "■ 再生を停止"
      : playButton.dataset.labelPlay || "▶ 波形を再生";
  }

  function stopPlayback(useFade = true) {
    window.clearTimeout(stopTimer);
    stopTimer = 0;
    if (!activeGraph || !audioContext) {
      setPlayingState(false);
      return;
    }
    const graph = activeGraph;
    activeGraph = null;
    const now = audioContext.currentTime;
    if (useFade) {
      graph.master.gain.cancelScheduledValues(now);
      graph.master.gain.setValueAtTime(graph.master.gain.value, now);
      graph.master.gain.linearRampToValueAtTime(0, now + 0.025);
    }
    try { graph.source.stop(now + (useFade ? 0.03 : 0)); } catch (error) {}
    window.setTimeout(() => {
      Object.values(graph).forEach((node) => {
        if (node && typeof node.disconnect === "function") {
          try { node.disconnect(); } catch (error) {}
        }
      });
    }, 60);
    setPlayingState(false);
  }

  async function startPlayback() {
    if (activeGraph) {
      stopPlayback();
      return;
    }
    audioContext ||= new (window.AudioContext || window.webkitAudioContext)();
    await audioContext.resume();

    const current = values();
    const buffer = audioContext.createBuffer(1, renderedGrain.length, SAMPLE_RATE);
    buffer.copyToChannel(renderedGrain, 0);

    const source = audioContext.createBufferSource();
    const delayInput = audioContext.createGain();
    const clipper = audioContext.createWaveShaper();
    const delay = audioContext.createDelay(0.704);
    const feedback = audioContext.createGain();
    const wet = audioContext.createGain();
    const master = audioContext.createGain();

    clipper.curve = new Float32Array([-1, 1]);
    clipper.oversample = "none";
    source.buffer = buffer;
    delay.delayTime.value = current.delaySeconds;
    feedback.gain.value = Math.sqrt(current.feedback) * 0.96;
    wet.gain.value = current.mix;
    master.gain.value = current.volume;

    source.connect(master);
    source.connect(delayInput);
    delay.connect(feedback);
    feedback.connect(delayInput);
    delayInput.connect(clipper);
    clipper.connect(delay);
    delay.connect(wet);
    wet.connect(master);
    master.connect(audioContext.destination);

    activeGraph = { source, delayInput, clipper, delay, feedback, wet, master, startedAt: audioContext.currentTime };
    source.start();
    setPlayingState(true);
    schedulePlaybackStop();
  }

  function drawWaveform() {
    const context = canvas.getContext("2d");
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    const width = Math.max(1, canvas.clientWidth);
    const height = Math.max(1, canvas.clientHeight);
    canvas.width = Math.round(width * ratio);
    canvas.height = Math.round(height * ratio);
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, width, height);

    context.strokeStyle = "rgba(236,238,233,.08)";
    context.lineWidth = 1;
    for (let row = 1; row < 4; row += 1) {
      const y = row * height / 4;
      context.beginPath();
      context.moveTo(0, y);
      context.lineTo(width, y);
      context.stroke();
    }
    for (let column = 1; column < 8; column += 1) {
      const x = column * width / 8;
      context.beginPath();
      context.moveTo(x, 0);
      context.lineTo(x, height);
      context.stroke();
    }

    const maximum = renderedGrain.reduce((peak, value) => Math.max(peak, Math.abs(value)), 0.001);
    context.strokeStyle = "#72d694";
    context.lineWidth = 1.35;
    context.beginPath();
    for (let x = 0; x < width; x += 1) {
      const index = Math.min(renderedGrain.length - 1, Math.floor(x / Math.max(1, width - 1) * renderedGrain.length));
      const y = height / 2 - renderedGrain[index] / maximum * (height * 0.38);
      if (x === 0) context.moveTo(x, y);
      else context.lineTo(x, y);
    }
    context.stroke();
  }

  Object.values(controls).forEach((control) => control.addEventListener("input", applyLiveParameters));
  playButton.addEventListener("click", startPlayback);
  window.addEventListener("resize", drawWaveform);
  document.addEventListener("visibilitychange", () => {
    if (document.hidden) stopPlayback();
  });
  document.addEventListener("languagechange", () => setPlayingState(Boolean(activeGraph)));

  refreshLabels();
  drawWaveform();
})();
