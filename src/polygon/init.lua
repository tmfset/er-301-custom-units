local units = {
  { category = "Polyphonic Saw" },
  { title = "1 Voice", moduleName = "Saw.OneVoice", keywords = "saw, source, mono, monophonic, synth, macro" },
  { title = "3 Voice", moduleName = "Saw.ThreeVoice", keywords = "saw, source, poly, polyphonic, synth, macro, bank" },
  { title = "4 Voice", moduleName = "Saw.FourVoice", keywords = "saw, source, poly, polyphonic, synth, macro, bank" },
  { title = "6 Voice", moduleName = "Saw.SixVoice", keywords = "saw, source, poly, polyphonic, synth, macro, bank" },

  { category = "Polyphonic Triangle" },
  { title = "1 Voice", moduleName = "Triangle.OneVoice", keywords = "triangle, source, mono, monophonic, synth, macro" },
  { title = "3 Voice", moduleName = "Triangle.ThreeVoice", keywords = "triangle, source, poly, polyphonic, synth, macro, bank" },
  { title = "4 Voice", moduleName = "Triangle.FourVoice", keywords = "triangle, source, poly, polyphonic, synth, macro, bank" },
  { title = "6 Voice", moduleName = "Triangle.SixVoice", keywords = "triangle, source, poly, polyphonic, synth, macro, bank" },

  { category = "Polyphonic Single Cycle" },
  { title = "1 Voice", moduleName = "SingleCycle.OneVoice", keywords = "mono, source, monophonic, synth, macro" },
  { title = "3 Voice", moduleName = "SingleCycle.ThreeVoice", keywords = "poly, source, polyphonic, synth, macro, bank" },
  { title = "4 Voice", moduleName = "SingleCycle.FourVoice", keywords = "poly, source, polyphonic, synth, macro, bank" },
  { title = "6 Voice", moduleName = "SingleCycle.SixVoice", keywords = "poly, source, polyphonic, synth, macro, bank" }
}

return {
  title = "Simple Synthesizers",
  contact = "tomjfiset@gmail.com",
  keyword = "source, saw, triangle, poly, polyphonic, synth, macro, bank",
  units = units
}
