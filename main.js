import Poco from "commodetto/Poco";

const render = new Poco(screen);

const bigFont = new render.Font("Bitham-Bold", 42);
const smallFont = new render.Font("Gothic-Bold", 18);

const lcdBg = render.makeColor(182, 192, 140);
const lcdFg = render.makeColor(24, 28, 18);

const black = render.makeColor(0, 0, 0);
const bezelBody = render.makeColor(48, 50, 54);
const bezelHi = render.makeColor(72, 76, 82);
const blueLine = render.makeColor(20, 50, 140);
const white = render.makeColor(220, 220, 220);
const dimGray = render.makeColor(90, 92, 96);
const casioRed = render.makeColor(170, 35, 30);
const casioBlue = render.makeColor(25, 55, 135);

const W = render.width;
const H = render.height;
const DAYS = ["SU", "MO", "TU", "WE", "TH", "FR", "SA"];

const lcdX = 18;
const lcdY = 56;
const lcdW = W - 36;
const lcdH = 120;

function draw(event) {
    const now = event.date;

    render.begin();

    render.fillRectangle(black, 0, 0, W, H);
    render.fillRectangle(bezelBody, 4, 4, W - 8, H - 8);
    render.fillRectangle(bezelHi, 6, 6, W - 12, 1);
    render.fillRectangle(bezelHi, 6, 6, 1, H - 12);

    // Bezel top labels
    let w = render.getTextWidth("CASIO", smallFont);
    render.drawText("CASIO", smallFont, white, (W - w) / 2, 8);
    render.drawText("LIGHT", smallFont, dimGray, 10, 8);
    w = render.getTextWidth("F-91W", smallFont);
    render.drawText("F-91W", smallFont, dimGray, W - 10 - w, 8);

    w = render.getTextWidth("ALARM CHRONOGRAPH", smallFont);
    render.drawText("ALARM CHRONOGRAPH", smallFont, dimGray, (W - w) / 2, 34);

    // Blue border + LCD
    const bp = 2;
    render.fillRectangle(blueLine, lcdX - bp, lcdY - bp, lcdW + bp * 2, lcdH + bp * 2);
    render.fillRectangle(lcdBg, lcdX, lcdY, lcdW, lcdH);

    // Day + Date
    const topY = lcdY + 6;
    render.drawText(DAYS[now.getDay()], smallFont, lcdFg, lcdX + 8, topY);
    const dateStr = String(now.getDate()).padStart(2, "0");
    w = render.getTextWidth(dateStr, smallFont);
    render.drawText(dateStr, smallFont, lcdFg, lcdX + lcdW - 8 - w, topY);

    // Divider
    const div1Y = topY + smallFont.height + 4;
    render.fillRectangle(lcdFg, lcdX + 3, div1Y, lcdW - 6, 1);

    // Time centered, seconds to the right
    const hours = String(now.getHours()).padStart(2, "0");
    const minutes = String(now.getMinutes()).padStart(2, "0");
    const timeStr = `${hours}:${minutes}`;
    const timeW = render.getTextWidth(timeStr, bigFont);
    const secStr = String(now.getSeconds()).padStart(2, "0");
    const secW = render.getTextWidth(secStr, smallFont);

    // Center the whole group (time + gap + seconds)
    const gap = 4;
    const totalW = timeW + gap + secW;
    const startX = lcdX + (lcdW - totalW) / 2;
    const timeY = div1Y + 8;

    render.drawText(timeStr, bigFont, lcdFg, startX, timeY);

    // Seconds aligned to bottom of time digits
    const secY = timeY + bigFont.height - smallFont.height;
    render.drawText(secStr, smallFont, lcdFg, startX + timeW + gap, secY);

    // Bottom bezel: WATER WR RESIST
    const botY = lcdY + lcdH + bp + 10;
    render.drawText("WATER", smallFont, dimGray, 14, botY);
    const wrX = (W / 2) - 12;
    render.drawText("W", smallFont, casioRed, wrX, botY);
    render.drawText("R", smallFont, casioBlue, wrX + render.getTextWidth("W", smallFont) + 1, botY);
    w = render.getTextWidth("RESIST", smallFont);
    render.drawText("RESIST", smallFont, dimGray, W - 14 - w, botY);

    render.end();
}

watch.addEventListener("minutechange", draw);
