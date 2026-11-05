# TK VOCAL STUDIO

Plugin xử lý vocal (VST3, Windows 64-bit) viết bằng C++/JUCE 8. Toàn bộ DSP chạy thật, không có nút giả.

## Trạng thái kiểm tra (đọc trước)

| Hạng mục | Trạng thái |
|---|---|
| Lõi DSP (`Source/DSP.h`) | **Đã test offline** (`tests/dsp_test.cpp`) ở 44.1/48/88.2/96/192 kHz: giữ mức, L/R, mono, pha, bypass, level match, không NaN, không vượt ceiling limiter |
| Code JUCE (Processor/Editor) | **Mới kiểm tra cú pháp** với JUCE 8.0.4; chưa link/chạy |
| Build VST3 Windows | **Chưa build** (được viết trong môi trường Linux, không có MSVC) |
| Chạy trong FL Studio, giao diện, CPU thực tế | **Chưa test** — bạn cần build và thử trên máy Windows |

Vì vậy hãy xem đây là bản đầu hoàn chỉnh về mã nguồn, chưa phải bản đã qua kiểm thử trên host.

## Build và cài đặt

Yêu cầu: Windows 10/11 64-bit, Visual Studio 2022 (workload *Desktop development with C++*), CMake ≥ 3.22, Git.

1. Chạy `build.bat` — tự tải JUCE 8.0.4 vào `extern\JUCE`, cấu hình và build Release.
2. Chuột phải `install.bat` → *Run as administrator* — chép vào `C:\Program Files\Common Files\VST3\TK VOCAL STUDIO.vst3`.
3. FL Studio: *Options → Manage plugins → Find installed plugins*, rồi tìm **TK VOCAL STUDIO** (hoặc Add → Effects).
4. `run.bat` mở bản Standalone để thử nhanh (dùng ASIO/WASAPI của JUCE).

Không có DLL phụ trợ: toàn bộ DSP nằm trong chính bundle `.vst3`.

JUCE 8 dùng giấy phép AGPLv3 hoặc thương mại. Nếu bạn phát hành bản đóng, cần giấy phép thương mại của JUCE.

## Chuỗi xử lý

Input Gain → Stereo (L/R gain, đảo pha, Separation, Mid/Side + Width, Balance) → HPF/LPF 24 dB/oct → Noise Reduction → EQ 7 band → Vocal Clarity/Presence/Warmth/Air → De-esser → Dynamic EQ → Transient → Compressor → Multiband (LR4, 3 băng) → Exciter → Level Match → Output Gain → Limiter (lookahead) → Peak Protection → Bypass/Mono check.

- **EQ**: 7 band đúng dải yêu cầu; mỗi band có On, Freq, Gain, Q và loại Bell / Low Shelf / High Shelf / Notch. Có HPF/LPF, Reset EQ, 7 EQ preset, đường cong EQ vẽ lên analyzer.
- **Stereo**: Left/Right Gain, Balance (không tăng âm ở giữa), Width, Mid/Side gain, Separation (mono giữa không đổi), đảo pha từng kênh, Mono Check, Phase Correlation, Mono loss (dB), analyzer L/R riêng. EQ và các module khác xử lý chung cho hai kênh (không có EQ riêng cho từng kênh).
- **Mỗi module** có nút ON/OFF và thông số riêng (xem tab VOCAL và DYNAMICS).
- **Giữ âm lượng**: Input/Output Gain riêng; mọi gain mặc định 0 dB; chuỗi mặc định giữ nguyên mức (test: lệch < 0.05 dB). Compressor có Makeup chỉnh tay; Auto Makeup tùy chọn và chỉ tăng. Limiter chỉ tác động khi vượt Ceiling (mặc định −0.3 dB). Level Match (tùy chọn, mặc định tắt) kéo mức ra về mức vào, giới hạn ±12 dB, đo chậm (~0.4 s) để tránh pumping. Có peak meter vào/ra và LUFS ngắn hạn xấp xỉ (K-weighting gần đúng, không phải thiết bị đo chuẩn).
- **15 preset** đặt thông số DSP thật (EQ, module, ngưỡng, ratio...); không đổi gain vào/ra và cài đặt stereo của bạn.
- **A/B**, **Undo/Redo** (UndoManager của APVTS), **Bypass** (crossfade, đã bù độ trễ).

## Độ trễ và hiệu năng

- Độ trễ cố định 1.5 ms (lookahead của limiter), báo cho host qua `setLatencySamples` để FL Studio bù. Không dùng oversampling.
- Không cấp phát bộ nhớ trong `processBlock`; bộ lọc là biquad tự viết, tham số cập nhật theo block.
- Test offline với toàn bộ module bật (1 lõi, build -O2): khoảng 3% ở 44.1/48 kHz, 7% ở 96 kHz, 22% ở 192 kHz. Chỉ là con số tham khảo.

## Chạy test DSP (không cần JUCE)

```
g++ -std=c++17 -O2 -o dsp_test tests/dsp_test.cpp && ./dsp_test
```

## Cấu trúc

```
CMakeLists.txt      build.bat  install.bat  run.bat
Source/DSP.h        lõi DSP (không phụ thuộc JUCE)
Source/Params.h     toàn bộ tham số (APVTS)
Source/PluginProcessor.*  ghép tham số ↔ DSP, preset, A/B, trạng thái
Source/PluginEditor.* , UI.h  giao diện neon/kính tối, analyzer, meter
tests/dsp_test.cpp
```
