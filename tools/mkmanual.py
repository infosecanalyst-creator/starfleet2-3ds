#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 the starfleet2-3ds authors
"""Build the in-game manual (manual.txt) from the port guide plus the game's own manuals.

The game manuals come from YOUR GOG install (docs/Manuals/2.0/*.pdf); their text belongs
to the game's author, so manual.txt is generated on your machine for your SD card and is
never committed to the repository.

usage: mkmanual.py PORT_GUIDE.txt GOG_INSTALL_DIR OUT.txt
Sections start with "# Title"; the 3DS viewer word-wraps the text.
"""
import re
import sys

import pdfplumber

MAX_TITLE = 52          # fits a full-width manual button (320 px) in the 6x12 font


def columns(line):
    """A printed line, with two spaces where the page leaves a wide gap between words (table
    columns), so the cleaner keeps tables as rows instead of running them into prose."""
    chars = [c for c in line.get('chars', []) if c['text'].strip()]
    if not chars:
        return line['text']
    out, prev = '', None
    for c in chars:
        if prev is not None:
            gap = c['x0'] - prev['x1']
            w = max(prev['x1'] - prev['x0'], 3)
            if gap > w * 2.2:
                out += '  '
            elif gap > w * 0.25:
                out += ' '
        out += c['text']
        prev = c
    return out


def pdf_text(path):
    """Page text, one line per printed line, with an empty line wherever the printed page leaves
    extra space (a new paragraph, heading or table), so paragraphs survive into the manual."""
    out = []
    with pdfplumber.open(path) as p:
        for pg in p.pages:
            lines = pg.extract_text_lines() or []
            gaps = sorted(b['top'] - a['top'] for a, b in zip(lines, lines[1:]) if b['top'] > a['top'])
            step = gaps[len(gaps) // 4] if gaps else 12        # the usual line-to-line distance
            prev = None
            for L in lines:
                if prev is not None and L['top'] - prev['top'] > step * 1.5:
                    out.append('')
                out.append(columns(L))
                prev = L
            out.append('')
    return '\n'.join(out)


def quick_ref(text):
    """The Quick Reference card is a two-column table (key, what it does | key, what it does):
    list the left column, then the right, one key per line."""
    left, right, other = [], [], []
    for L in text.split('\n'):
        cells = [c.strip() for c in re.split(r'\s{2,}', L.strip()) if c.strip()]
        if not cells or cells[0] == 'KEYSTROKE':
            continue
        if len(cells) >= 4 and cells[0].startswith('[') and cells[2].startswith('['):
            left.append(cells[0] + '  ' + cells[1]); right.append(cells[2] + '  ' + ' '.join(cells[3:]))
        elif len(cells) == 3 and cells[0].startswith('['):
            if cells[1].startswith('['):        # left key whose text went missing
                left.append(cells[0]); right.append(cells[1] + '  ' + cells[2])
            else:                                # left entry + a line continuing the right one
                left.append(cells[0] + '  ' + cells[1])
                if right: right[-1] += ' ' + cells[2]
                else: right.append(cells[2])
        elif len(cells) == 2 and cells[0].startswith('['):
            left.append(cells[0] + '  ' + cells[1])
        elif right and not cells[0].startswith('['):
            right[-1] += ' ' + ' '.join(cells)    # a wrapped description
        else:
            other.append(' '.join(cells))
    out = left + right
    if other:
        out += [''] + other
    return '\n'.join(out)


def title_case(s):
    small = {'a', 'an', 'and', 'of', 'the', 'to', 'in', 'on', 'for', 'or', 'with'}
    words = s.split()
    out = []
    for i, w in enumerate(words):
        if w.startswith('[') or any(ch.isdigit() for ch in w):
            out.append(w)
        elif i and w.lower() in small and not any(ch.isdigit() for ch in words[i - 1]):
            out.append(w.lower())
        else:
            out.append(w[:1].upper() + w[1:].lower())
    return ' '.join(out)


def short_title(t):
    t = re.sub(r'\s*\[[^\]]*\]', '', t)          # drop key paths like [E] / [Fl - F8]
    t = re.sub(r'\s*/.*$', '', t)                # and anything after a slash
    t = t.strip(' .:-')
    if len(t) <= MAX_TITLE:
        return t
    words, out = t.split(), ''
    for w in words:                              # shorten at a word boundary, never mid-word
        if len(out) + len(w) + (1 if out else 0) > MAX_TITLE:
            break
        out = (out + ' ' + w).strip()
    return out


def clean(lines, header_res):
    out = []
    for L in lines:
        s = L.rstrip()
        if any(r.match(' '.join(s.split())) for r in header_res):   # (page headers, spacing ignored)
            continue
        out.append(s)
    # join hard-wrapped lines into paragraphs; keep blank lines and table-like lines
    paras, cur = [], ''
    for s in out:
        if not s.strip():
            if cur:
                paras.append(cur); cur = ''
            paras.append('')
            continue
        tableish = '  ' in s.strip() or re.match(r'^(\[|•|-|\d+\)|[A-Z]\))', s.strip())
        if tableish:
            if cur:
                paras.append(cur); cur = ''
            paras.append(s.strip())
        else:
            cur = (cur + ' ' + s.strip()).strip() if cur else s.strip()
            if s.strip().endswith(('.', ':', '!', '?')) and len(s) < 70:
                paras.append(cur); cur = ''
    if cur:
        paras.append(cur)
    # one empty line between paragraphs (the manual draws it as a gap); none inside tables
    res = []
    for p in paras:
        if p == '':
            if res and res[-1] != '':
                res.append('')
            continue
        res.append(p)
    while res and res[-1] == '':
        res.pop()
    return res


def split_sections(text, head_re, body_start_re, prefix, header_res):
    lines = text.split('\n')
    # body starts at the first heading that is not a table-of-contents entry (no page number)
    start = 0
    for i, L in enumerate(lines):
        if body_start_re.match(L.strip()) and not re.search(r'(\.{3,}|\s\d+)$', L.strip()):
            start = i
            break
    sections, title, buf = [], None, []
    for L in lines[start:]:
        m = head_re.match(L.strip())
        if m and not re.search(r'(\.{3,})\s*\d+$', L.strip()):
            if title:
                sections.append((title, clean(buf, header_res)))
            title = short_title(prefix + title_case(m.group(1)))
            buf = []
        else:
            buf.append(L)
    if title:
        sections.append((title, clean(buf, header_res)))
    return sections


def main():
    guide, gog, out = sys.argv[1], sys.argv[2], sys.argv[3]
    man = gog.rstrip('/\\') + '/docs/Manuals/2.0/'
    parts = [open(guide, encoding='utf-8').read().rstrip() + '\n']

    qr = pdf_text(man + 'Star Fleet II Quick Reference Guide.pdf')
    parts.append('# Keyboard Quick Reference\nThe game\'s own keyboard shortcuts (the port presses these for you).\n\n' + quick_ref(qr) + '\n')

    ops = pdf_text(man + 'Klagar-Class Battlecruiser Operations Manual.pdf')
    ops_hdr = [re.compile(r'^\d+ KLAGAR-CLASS BATTLECRUISER OPS MANUAL\s*$'),
               re.compile(r'^KLAGAR-CLASS BATTLECRUISER OPS MANUAL \d+\s*$'),
               re.compile(r'^.*OPS MANUAL\s+\d+\s*$')]
    for t, body in split_sections(ops, re.compile(r'^(\d+\.\d+\s+[A-Z][A-Za-z].*)$'),
                                  re.compile(r'^1\.0 SHIP SECTIONS'), 'Ops ', ops_hdr):
        parts.append('# ' + t + '\n' + '\n'.join(body).strip() + '\n')

    tm = pdf_text(man + 'Star Fleet Training Manual Volume 2 Version 2.pdf')
    tm_hdr = [re.compile(r'^\d+ STAR FLEET TRAINING MANUAL.*$'), re.compile(r'^STAR FLEET TRAINING MANUAL.*\d+\s*$')]
    tut_names = dict(re.findall(r'^Tutorial #(\d+): (.*?) \d+$', tm, re.M))
    tut_names['3'] = 'Teleporters 2: Material & Personnel'   # full title is too long for a button
    for t, body in split_sections(tm, re.compile(r'^((?:\d+\.\d+\s+[A-Z][A-Za-z].*)|(?:TUTORIAL #\d+))$'),
                                  re.compile(r'^2\.0 YOUR MISSIONS'), 'Training ', tm_hdr):
        m = re.match(r'Training Tutorial #(\d+)', t)
        if m:
            t = short_title(f'Tutorial {m.group(1)}: ' + tut_names.get(m.group(1), ''))
        parts.append('# ' + t + '\n' + '\n'.join(body).strip() + '\n')

    text = '\n'.join(parts)
    # the 3DS font is CP437: replace characters it cannot show
    repl = {'‘': "'", '’': "'", '“': '"', '”': '"', '–': '-', '—': '-',
            '…': '...', '•': '•', ' ': ' '}
    for a, b in repl.items():
        text = text.replace(a, b)
    data = text.encode('cp437', 'replace')
    open(out, 'wb').write(data)
    n = sum(1 for L in text.split('\n') if L.startswith('# '))
    print(f'wrote {out}: {n} sections, {len(data)} bytes')


if __name__ == '__main__':
    main()
