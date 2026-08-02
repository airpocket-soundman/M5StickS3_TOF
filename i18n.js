(() => {
  "use strict";

  const translations = {
    en: {
      "自然現象を、": "Turn natural phenomena into",
      "物理オシレーターに。": "physical oscillators.",
      "Water, sensing and computation form a physical–computational oscillator that generates sound grains.": "Water, sensing, and computation form one oscillating system that generates sound grains.",
      "自然を録音するのではない。": "Do not record nature.",
      "自然と計算で発振する。": "Oscillate with nature and computation.",
      "マイクが捉えるのは、すでに音になった自然です。RIPPLE OSCILLATORが観測するのは、その手前にある動き。風に揺れる葉、水面を走る波紋、海のうねりを、変位の時系列として読み取ります。": "A microphone captures nature after it has become sound. RIPPLE OSCILLATOR observes the motion that comes before it: leaves moving in the wind, ripples crossing water, and ocean swells, read as time-series displacement.",
      "RIPPLE OSCILLATORは、水面、ToFセンサ、信号処理をひとつの": "RIPPLE OSCILLATOR combines a water surface, a ToF sensor, and signal processing into a single ",
      "物理–計算オシレーター（以下、物理オシレーター）": "physical–computational oscillator (physical oscillator)",
      "として構成します。水面が振動を生み、ToFが一点の変位を波形へ変え、プロセッサが2秒の観測窓を短い音源グレインへ変換する。自然と計算機が協働して音源を生成する仕組みです。": ". The surface produces vibration, ToF converts displacement at one point into a waveform, and the processor transforms a two-second observation window into a short source grain. Nature and computation generate the source together.",
      "不可視の電波を色へ写し替えた天体画像。観測不能な領域を人間の感覚へ写像するという方法を、水面と聴覚のあいだに展開する。Credit: NASA, ESA, S. Baum, C. O'Dea, R. Perley, W. Cotton and the Hubble Heritage Team.": "An astronomical image translating invisible radio waves into color. Its method—mapping an imperceptible domain into human sensation—is extended here between the water surface and hearing. Credit: NASA, ESA, S. Baum, C. O'Dea, R. Perley, W. Cotton and the Hubble Heritage Team.",
      "電磁場の観測": "Observing electromagnetic fields",
      "スペクトル写像": "Spectrum mapping",
      "＋ 擬似カラー割り当て": "+ false-color assignment",
      "観測データ → 可視表現": "observed data → visual form",
      "可視領域への写像": "Mapping into the visible field",
      "表面変位の観測": "Observing surface displacement",
      "単点測距": "single-point ranging",
      "波形整形": "Waveform shaping",
      "＋ 時間軸・ピッチ変換": "+ time-axis and pitch conversion",
      "表面変位 → 音源グレイン": "surface displacement → source grain",
      "音源グレインへの写像": "Mapping into a source grain",
      "実測 / GRAIN 1": "MEASURED / GRAIN 1",
      "物理オシレーターは、": "A physical oscillator begins",
      "自然の振動から始まる。": "with vibrations in nature.",
      "葉の振動": "Leaf vibration",
      "風を受けた葉の数Hzの運動。センサと信号処理を接続すれば、物理オシレーターの振動体になる。": "A leaf moves at a few hertz in the wind. Connected to sensing and signal processing, it becomes the vibrating body of a physical oscillator.",
      "水面の波紋": "Ripples on water",
      "一滴から生まれ、干渉し、減衰する変位。本研究ではこの時間構造から音源グレインを生成する。": "Displacement born from one drop, interfering and decaying. This study generates source grains from that temporal structure.",
      "海のうねり": "Ocean swells",
      "数秒から十数秒周期で動く大きな系も、観測と変換を組み合わせれば固有の発振系になり得る。": "Even large systems moving in cycles of several to tens of seconds can become distinct oscillating systems through observation and translation.",
      "水面・ToF・プロセッサを、": "Water, ToF, and processor:",
      "ひとつの発振系に。": "one oscillating system.",
      "振動体である水面、単一点の変位を読むToFセンサ、波形を可聴化するプロセッサ。この三者を統合した系全体を、RIPPLE OSCILLATORでは「物理–計算オシレーター」と呼びます。": "RIPPLE OSCILLATOR calls the complete system a physical–computational oscillator: the water surface as vibrating body, a ToF sensor reading displacement at one point, and a processor making the waveform audible.",
      "雫の着水を検出すると、プリロールを含む2秒間を切り出し、時間圧縮とフェーズボコーダで16kHz・16-bit PCMへ変換します。その短い出力単位が、シンセサイザへ供給する音源グレインです。": "When a drop lands, the system captures two seconds including pre-roll, then converts it to 16 kHz, 16-bit PCM through time compression and a phase vocoder. This short output unit becomes the source grain supplied to the synthesizer.",
      "内蔵オシレーター": "Internal oscillator",
      "水面＋ToF＋プロセッサ": "Water + ToF + processor",
      "雫が水面を励起し、干渉しながら減衰する物理的な振動を生む。": "A drop excites the surface, producing physical vibrations that interfere and decay.",
      "ToFセンサが水面上の一点を測距し、2秒間の変位波形を取得。": "The ToF sensor ranges one point on the surface and captures two seconds of displacement.",
      "時間圧縮とピッチ変換により、変位波形からPCMグレインを生成。": "Time compression and pitch conversion turn the displacement waveform into a PCM grain.",
      "UDPでTab5へ送り、ボイス別ADSRとエフェクトを加えて演奏。": "Send it to Tab5 over UDP, then perform with per-voice ADSR and effects.",
      "物理オシレーターの観測面。": "The physical oscillator's observation surface.",
      "M5StickS3の画面に描かれる緑の線は、実測された水面変位です。赤はグレインの記録中、緑は生成したグレインの発音中。水面、観測、変換が一つの音源系として動作します。": "The green line on the M5StickS3 shows measured water-surface displacement. Red indicates grain recording; green indicates playback of the generated grain. Surface, observation, and translation operate as one source system.",
      "音源グレインを、": "Route the source grain",
      "シンセの信号経路へ。": "into the synth signal path.",
      "物理オシレーターのグレインを聴く": "Listen to the physical oscillator grain",
      "▶ 波形を再生": "▶ Play waveform",
      "音量": "Volume",
      "ディレイ時間": "Delay time",
      "フィードバック": "Feedback",
      "ディレイ量": "Delay mix",
      "物理オシレーターが実測波形から生成したグレインを、現ファームと同じ0.2×時間圧縮・振幅依存ピッチでレンダリングしています。これらは固定し、Tab5のストリーム用Delayと音量だけをブラウザー上で操作できます。": "This grain, generated by the physical oscillator from a measured waveform, is rendered with the firmware's 0.2× time compression and amplitude-dependent pitch. Those parameters are fixed; only Tab5's stream delay and volume are adjustable here.",
      "GP2Y0E03 / 1kHzポーリング / 500Hz波形記録 / 赤外線・非接触": "GP2Y0E03 / 1 kHz polling / 500 Hz waveform capture / infrared, contactless",
      "エンベロープ比較によるオンセット検出 / 150msプリロール / 2秒キャプチャ": "Onset detection by envelope comparison / 150 ms pre-roll / 2 s capture",
      "時間圧縮0.2倍 / 1024点フェーズボコーダ / +12〜+36半音": "0.2× time compression / 1,024-point phase vocoder / +12 to +36 semitones",
      "WiFi UDP / ボイスID付き独自プロトコル / 最大4ストリーム": "Wi-Fi UDP / custom protocol with voice ID / up to four streams",
      "M5Stack Tab5 / ボイス別ADSR・ミックス / フィルター・コーラス・ディレイ・歪み・ビットクラッシャー": "M5Stack Tab5 / per-voice ADSR and mix / filter, chorus, delay, distortion, bit crusher",
      "物理オシレーターを、音源回路へ接続する。": "Connect the physical oscillator to the source circuit.",
      "一般的なシンセサイザでは、内蔵オシレーターがサイン波・ノコギリ波・矩形波などを生成します。RIPPLE OSCILLATORでは、その音源位置へ": "A conventional synthesizer uses internal oscillators to generate sine, sawtooth, square, and other waves. RIPPLE OSCILLATOR instead supplies ",
      "物理–計算オシレーターが生成した短いグレイン": "short grains generated by a physical–computational oscillator",
      "を供給します。PCMをUDPで受け取ったTab5は、最大4ボイスを個別にADSR処理してミックスし、フィルター、コーラス、ディレイ、歪み、ビットクラッシャーを適用します。これは自然音の録音や模倣ではなく、物理現象を含む発振系によるデジタル音源です。": " at that source position. Tab5 receives PCM over UDP, applies ADSR to as many as four voices, mixes them, and adds filter, chorus, delay, distortion, and bit crushing. This is not a recording or imitation of natural sound, but a digital source produced by an oscillating system that includes a physical phenomenon.",
      "水面は、": "The water surface is",
      "始まりにすぎない。": "only the beginning.",
      "葉の揺れ、旗のはためき、海のうねり。変位として観測できる現象なら、センサと信号処理を組み合わせて物理–計算オシレーターを構成できます。自然をデジタル空間へ複製するのではなく、自然と計算機がひとつの発振系を共有する。その系が生むグレインから、まだ存在しない音を立ち上げます。": "Leaves swaying, flags fluttering, ocean swells—any phenomenon observable as displacement can form a physical–computational oscillator with sensing and signal processing. Rather than copying nature into digital space, nature and computer share one oscillating system. Its grains bring forth sounds that have never existed before."
    },
    zh: {
      "自然現象を、": "让自然现象成为",
      "物理オシレーターに。": "物理振荡器。",
      "Water, sensing and computation form a physical–computational oscillator that generates sound grains.": "水、传感与计算共同构成物理–计算振荡器，并由此生成声音颗粒。",
      "自然を録音するのではない。": "不是录制自然，",
      "自然と計算で発振する。": "而是让自然与计算共同振荡。",
      "マイクが捉えるのは、すでに音になった自然です。RIPPLE OSCILLATORが観測するのは、その手前にある動き。風に揺れる葉、水面を走る波紋、海のうねりを、変位の時系列として読み取ります。": "麦克风捕捉的是已经成为声音的自然。RIPPLE OSCILLATOR 观测的是声音之前的运动：风中的树叶、水面的涟漪与海浪，都被读取为位移的时间序列。",
      "RIPPLE OSCILLATORは、水面、ToFセンサ、信号処理をひとつの": "RIPPLE OSCILLATOR 将水面、ToF 传感器与信号处理构成一个",
      "物理–計算オシレーター（以下、物理オシレーター）": "物理–计算振荡器（以下简称物理振荡器）",
      "として構成します。水面が振動を生み、ToFが一点の変位を波形へ変え、プロセッサが2秒の観測窓を短い音源グレインへ変換する。自然と計算機が協働して音源を生成する仕組みです。": "。水面产生振动，ToF 将单点位移转换为波形，处理器再把两秒的观测窗口变成短小的声音颗粒。自然与计算机由此共同生成声源。",
      "不可視の電波を色へ写し替えた天体画像。観測不能な領域を人間の感覚へ写像するという方法を、水面と聴覚のあいだに展開する。Credit: NASA, ESA, S. Baum, C. O'Dea, R. Perley, W. Cotton and the Hubble Heritage Team.": "这幅天文图像把不可见的无线电波转换成色彩。本项目将“把不可感知领域映射到人的感官”这一方法，延伸到水面与听觉之间。图片来源：NASA、ESA、S. Baum、C. O'Dea、R. Perley、W. Cotton 与 Hubble Heritage Team。",
      "電磁場の観測": "观测电磁场",
      "スペクトル写像": "频谱映射",
      "＋ 擬似カラー割り当て": "＋ 伪彩色分配",
      "観測データ → 可視表現": "观测数据 → 可视表达",
      "可視領域への写像": "映射到可见域",
      "表面変位の観測": "观测表面位移",
      "単点測距": "单点测距",
      "波形整形": "波形整形",
      "＋ 時間軸・ピッチ変換": "＋ 时间轴与音高转换",
      "表面変位 → 音源グレイン": "表面位移 → 声源颗粒",
      "音源グレインへの写像": "映射为声源颗粒",
      "実測 / GRAIN 1": "实测 / GRAIN 1",
      "物理オシレーターは、": "物理振荡器，",
      "自然の振動から始まる。": "始于自然的振动。",
      "葉の振動": "树叶的振动",
      "風を受けた葉の数Hzの運動。センサと信号処理を接続すれば、物理オシレーターの振動体になる。": "树叶在风中以数赫兹运动。连接传感器与信号处理后，它便成为物理振荡器的振动体。",
      "水面の波紋": "水面的涟漪",
      "一滴から生まれ、干渉し、減衰する変位。本研究ではこの時間構造から音源グレインを生成する。": "由一滴水产生、相互干涉并逐渐衰减的位移。本研究从这种时间结构中生成声源颗粒。",
      "海のうねり": "海浪的涌动",
      "数秒から十数秒周期で動く大きな系も、観測と変換を組み合わせれば固有の発振系になり得る。": "即使是以数秒至十几秒为周期运动的大型系统，也能通过观测与转换成为独特的振荡系统。",
      "水面・ToF・プロセッサを、": "让水面、ToF 与处理器",
      "ひとつの発振系に。": "组成一个振荡系统。",
      "振動体である水面、単一点の変位を読むToFセンサ、波形を可聴化するプロセッサ。この三者を統合した系全体を、RIPPLE OSCILLATORでは「物理–計算オシレーター」と呼びます。": "水面是振动体，ToF 传感器读取单点位移，处理器将波形转化为可听声音。RIPPLE OSCILLATOR 将三者整合而成的整体称为“物理–计算振荡器”。",
      "雫の着水を検出すると、プリロールを含む2秒間を切り出し、時間圧縮とフェーズボコーダで16kHz・16-bit PCMへ変換します。その短い出力単位が、シンセサイザへ供給する音源グレインです。": "检测到水滴落下后，系统截取包含预录的两秒信号，并通过时间压缩与相位声码器转换为 16 kHz、16-bit PCM。这个短输出单元就是提供给合成器的声源颗粒。",
      "内蔵オシレーター": "内置振荡器",
      "水面＋ToF＋プロセッサ": "水面＋ToF＋处理器",
      "雫が水面を励起し、干渉しながら減衰する物理的な振動を生む。": "水滴激发水面，产生相互干涉并逐渐衰减的物理振动。",
      "ToFセンサが水面上の一点を測距し、2秒間の変位波形を取得。": "ToF 传感器测量水面上的一点，获取两秒的位移波形。",
      "時間圧縮とピッチ変換により、変位波形からPCMグレインを生成。": "通过时间压缩与音高转换，从位移波形生成 PCM 颗粒。",
      "UDPでTab5へ送り、ボイス別ADSRとエフェクトを加えて演奏。": "通过 UDP 发送至 Tab5，加入逐声部 ADSR 与效果后演奏。",
      "物理オシレーターの観測面。": "物理振荡器的观测界面。",
      "M5StickS3の画面に描かれる緑の線は、実測された水面変位です。赤はグレインの記録中、緑は生成したグレインの発音中。水面、観測、変換が一つの音源系として動作します。": "M5StickS3 屏幕上的绿线表示实测水面位移。红色表示正在记录颗粒，绿色表示正在播放已生成的颗粒。水面、观测与转换作为一个声源系统运行。",
      "音源グレインを、": "将声源颗粒接入",
      "シンセの信号経路へ。": "合成器的信号路径。",
      "物理オシレーターのグレインを聴く": "聆听物理振荡器的颗粒",
      "▶ 波形を再生": "▶ 播放波形",
      "音量": "音量",
      "ディレイ時間": "延迟时间",
      "フィードバック": "反馈",
      "ディレイ量": "延迟混合量",
      "物理オシレーターが実測波形から生成したグレインを、現ファームと同じ0.2×時間圧縮・振幅依存ピッチでレンダリングしています。これらは固定し、Tab5のストリーム用Delayと音量だけをブラウザー上で操作できます。": "这里以当前固件相同的 0.2× 时间压缩和振幅相关音高，渲染物理振荡器从实测波形生成的颗粒。这些参数固定，仅可在浏览器中调整 Tab5 流的延迟与音量。",
      "GP2Y0E03 / 1kHzポーリング / 500Hz波形記録 / 赤外線・非接触": "GP2Y0E03 / 1 kHz 轮询 / 500 Hz 波形记录 / 红外、非接触",
      "エンベロープ比較によるオンセット検出 / 150msプリロール / 2秒キャプチャ": "通过包络比较检测起音 / 150 ms 预录 / 2 秒采集",
      "時間圧縮0.2倍 / 1024点フェーズボコーダ / +12〜+36半音": "0.2× 时间压缩 / 1024 点相位声码器 / +12 至 +36 半音",
      "WiFi UDP / ボイスID付き独自プロトコル / 最大4ストリーム": "Wi-Fi UDP / 带声部 ID 的自定义协议 / 最多 4 路流",
      "M5Stack Tab5 / ボイス別ADSR・ミックス / フィルター・コーラス・ディレイ・歪み・ビットクラッシャー": "M5Stack Tab5 / 逐声部 ADSR 与混音 / 滤波、合唱、延迟、失真、位深粉碎",
      "物理オシレーターを、音源回路へ接続する。": "将物理振荡器接入声源电路。",
      "一般的なシンセサイザでは、内蔵オシレーターがサイン波・ノコギリ波・矩形波などを生成します。RIPPLE OSCILLATORでは、その音源位置へ": "传统合成器由内置振荡器生成正弦波、锯齿波、方波等。RIPPLE OSCILLATOR 则在这一声源位置提供",
      "物理–計算オシレーターが生成した短いグレイン": "物理–计算振荡器生成的短颗粒",
      "を供給します。PCMをUDPで受け取ったTab5は、最大4ボイスを個別にADSR処理してミックスし、フィルター、コーラス、ディレイ、歪み、ビットクラッシャーを適用します。これは自然音の録音や模倣ではなく、物理現象を含む発振系によるデジタル音源です。": "。Tab5 通过 UDP 接收 PCM，对最多四个声部分别进行 ADSR 处理与混音，再应用滤波、合唱、延迟、失真和位深粉碎。这不是对自然声音的录制或模仿，而是由包含物理现象的振荡系统产生的数字声源。",
      "水面は、": "水面，",
      "始まりにすぎない。": "只是开始。",
      "葉の揺れ、旗のはためき、海のうねり。変位として観測できる現象なら、センサと信号処理を組み合わせて物理–計算オシレーターを構成できます。自然をデジタル空間へ複製するのではなく、自然と計算機がひとつの発振系を共有する。その系が生むグレインから、まだ存在しない音を立ち上げます。": "摇曳的树叶、飘动的旗帜、海浪的涌动——任何能以位移观测的现象，都可以结合传感器与信号处理，构成物理–计算振荡器。它不是把自然复制进数字空间，而是让自然与计算机共享一个振荡系统，并从系统产生的颗粒中唤起尚未存在的声音。"
    }
  };

  const attributes = {
    en: {
      "nav": { "aria-label": "Main navigation" },
      ".language-switcher": { "aria-label": "Display language" },
      ".cosmos-frame img": { "alt": "Radio galaxy Hercules A, with invisible radio jets composited over a visible-light image" },
      ".transduction-map": { "aria-label": "Translation map from natural phenomena to perceptual representation or source grain" },
      ".source-position": { "aria-label": "Comparison of an internal oscillator and a physical–computational oscillator" },
      ".process": { "aria-label": "Flow from physical oscillator to sound production" },
      ".device-figure img": { "alt": "Water-surface displacement waveform displayed on M5StickS3" },
      ".audio-controls": { "aria-label": "Volume and delay settings" },
      "#grain-wave": { "aria-label": "Measured grain waveform generated by the physical oscillator" },
      ".receiver-link": { "aria-label": "Open the Tab5_synth repository on GitHub" },
      ".receiver img": { "alt": "UDP input screen of the M5Stack Tab5 synthesizer" }
    },
    zh: {
      "nav": { "aria-label": "主导航" },
      ".language-switcher": { "aria-label": "显示语言" },
      ".cosmos-frame img": { "alt": "射电星系武仙座 A：不可见的射电喷流叠加在可见光图像上" },
      ".transduction-map": { "aria-label": "从自然现象到感知表达或声源颗粒的转换图" },
      ".source-position": { "aria-label": "内置振荡器与物理–计算振荡器的比较" },
      ".process": { "aria-label": "从物理振荡器到发声的流程" },
      ".device-figure img": { "alt": "M5StickS3 屏幕显示的水面位移波形" },
      ".audio-controls": { "aria-label": "音量与延迟设置" },
      "#grain-wave": { "aria-label": "物理振荡器生成的实测颗粒波形" },
      ".receiver-link": { "aria-label": "在 GitHub 打开 Tab5_synth 仓库" },
      ".receiver img": { "alt": "M5Stack Tab5 合成器的 UDP 输入界面" }
    }
  };

  const metadata = {
    ja: {
      title: "RIPPLE OSCILLATOR — Water Surface Sonification Study",
      description: "水面、ToFセンサ、信号処理を物理–計算オシレーターとして構成し、波紋から音源グレインを生成する研究 RIPPLE OSCILLATOR。"
    },
    en: {
      title: "RIPPLE OSCILLATOR — Water Surface Sonification Study",
      description: "RIPPLE OSCILLATOR combines a water surface, ToF sensing, and signal processing into a physical–computational oscillator that generates source grains from ripples."
    },
    zh: {
      title: "RIPPLE OSCILLATOR — 水面声音化研究",
      description: "RIPPLE OSCILLATOR 将水面、ToF 传感与信号处理构成物理–计算振荡器，从涟漪中生成声源颗粒。"
    }
  };

  const originalText = new WeakMap();
  const originalAttributes = new Map();
  const walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT, {
    acceptNode(node) {
      return ["SCRIPT", "STYLE"].includes(node.parentElement?.tagName)
        ? NodeFilter.FILTER_REJECT
        : NodeFilter.FILTER_ACCEPT;
    }
  });
  const textNodes = [];
  while (walker.nextNode()) {
    const node = walker.currentNode;
    originalText.set(node, node.nodeValue);
    textNodes.push(node);
  }

  document.querySelectorAll("[aria-label], img[alt]").forEach((element) => {
    const values = {};
    if (element.hasAttribute("aria-label")) values["aria-label"] = element.getAttribute("aria-label");
    if (element.hasAttribute("alt")) values.alt = element.getAttribute("alt");
    originalAttributes.set(element, values);
  });

  function translateText(node, language) {
    const original = originalText.get(node);
    const trimmed = original.trim();
    if (!trimmed) return;
    const translated = language === "ja" ? trimmed : translations[language]?.[trimmed];
    if (!translated) {
      node.nodeValue = original;
      return;
    }
    const leading = original.match(/^\s*/)[0];
    const trailing = original.match(/\s*$/)[0];
    node.nodeValue = leading + translated + trailing;
  }

  function setLanguage(language) {
    if (!metadata[language]) language = "ja";
    textNodes.forEach((node) => translateText(node, language));
    originalAttributes.forEach((values, element) => {
      Object.entries(values).forEach(([name, value]) => element.setAttribute(name, value));
    });
    if (language !== "ja") {
      Object.entries(attributes[language]).forEach(([selector, values]) => {
        const element = document.querySelector(selector);
        if (element) Object.entries(values).forEach(([name, value]) => element.setAttribute(name, value));
      });
    }
    document.documentElement.lang = language === "zh" ? "zh-CN" : language;
    document.title = metadata[language].title;
    document.querySelector('meta[name="description"]')?.setAttribute("content", metadata[language].description);
    document.querySelectorAll("[data-lang]").forEach((button) => {
      button.setAttribute("aria-pressed", String(button.dataset.lang === language));
    });
    const playButton = document.getElementById("grain-play");
    if (playButton) {
      const labels = {
        ja: ["▶ 波形を再生", "■ 再生を停止"],
        en: ["▶ Play waveform", "■ Stop playback"],
        zh: ["▶ 播放波形", "■ 停止播放"]
      }[language];
      playButton.dataset.labelPlay = labels[0];
      playButton.dataset.labelStop = labels[1];
    }
    try { localStorage.setItem("ripple-oscillator-language", language); } catch (error) {}
    document.dispatchEvent(new CustomEvent("languagechange", { detail: { language } }));
  }

  document.querySelectorAll("[data-lang]").forEach((button) => {
    button.addEventListener("click", () => setLanguage(button.dataset.lang));
  });

  let initialLanguage = "ja";
  try { initialLanguage = localStorage.getItem("ripple-oscillator-language") || "ja"; } catch (error) {}
  setLanguage(initialLanguage);
})();
