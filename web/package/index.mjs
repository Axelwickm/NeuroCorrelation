import createNeuroCorrelationModule from "./dist/neurocorrelation.mjs";

export async function mountNeuroCorrelation(options = {}) {
  const { canvas, preset = "STANDARD", seed, ...moduleOptions } = options;
  if (!(canvas instanceof HTMLCanvasElement)) {
    throw new Error("mountNeuroCorrelation requires an HTMLCanvasElement.");
  }

  const args = [];
  if (seed !== undefined) {
    args.push("--seed", String(seed));
  }
  args.push(String(preset));

  const module = await createNeuroCorrelationModule({
    ...moduleOptions,
    canvas,
    arguments: args,
    locateFile(path) {
      return new URL(`./dist/${path}`, import.meta.url).href;
    },
  });

  return {
    destroy() {
      if (typeof module._neurocorrelation_request_shutdown === "function") {
        module._neurocorrelation_request_shutdown();
      }
    },
    module,
  };
}

export default mountNeuroCorrelation;
