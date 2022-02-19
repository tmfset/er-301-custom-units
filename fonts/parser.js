var fs = require('fs');

function loadJson(fileName) {
  console.log(`Loading json from '${fileName}'`);
  return JSON.parse(fs.readFileSync(fileName, 'utf8'));
}

function printLetter(letter) {
  console.log(`${letter.code} -> `);
  letter.data.filter(v => v != 0).forEach(v => {
    const points = v.toString(2).padStart(16, " ").split("");
    const format = points.map(p => {
      return p == "1" ? "X" : " ";
    });
    const reversed = format.reverse().join("");
    console.log(reversed);
  });
}

const args = process.argv.slice(2);
const fileData = loadJson(args[0]);

const keys = Object.keys(fileData);
const letters = keys.flatMap(k => {
  return parseInt(k) ? [{
    code: k,
    data: fileData[k]
  }] : [];
})

letters.forEach(printLetter);
//printLetter(letters[20]);