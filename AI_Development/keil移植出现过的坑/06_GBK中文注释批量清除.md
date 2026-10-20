# GBK 中文注释批量清除（厂商 BSP → GCC/VSCode 工程收尾步骤）

> 场景：正点原子等厂商 BSP 源码为 GBK 编码，VSCode 默认 UTF-8 打开中文注释全部乱码。
> 用户（2026-08-20 LED_vscode 移植会话）明确要求：**移植后的代码不保留任何中文注释**。
> 实战工程：G:\...\3.HAL\transplant_test\LED_vscode（10 文件清洗，delay.c 最多 122 条）。

## 脚本设计（状态机，非正则逐行）

核心原则：
1. **只删含非 ASCII 字符的注释**——纯 ASCII 注释（`//PB8`、`//set Main Stack value`）保留，它们不乱码
2. **字符串/字符字面量绝不触碰**——`printf("温度")` 里的中文不能删，删了改变程序行为。逐字符状态机在 `NORMAL / LINE_COMMENT / BLOCK_COMMENT / STRING / CHAR` 间切换，只有注释态里累积的文本含非 ASCII 才整段丢弃
3. 清洗后文件重写为 **UTF-8 + LF**，连续空行压缩为 1 行（删注释后留下的大洞）

脚本骨架（完整可复用，实际使用时改 ROOT 遍历目录）：

```python
def strip_cn_comments(text):
    out, i, n = [], 0, len(text)
    state = 'N'  # N=normal L=line-comment B=block-comment S=string C=char
    buf = []
    while i < n:
        c = text[i]
        if state == 'N':
            if text.startswith('//', i): state, buf, i = 'L', ['//'], i+2
            elif text.startswith('/*', i): state, buf, i = 'B', ['/*'], i+2
            elif c == '"': state, i = 'S', i+1; out.append(c)
            elif c == "'": state, i = 'C', i+1; out.append(c)
            else: out.append(c); i += 1
        elif state == 'L':
            if c == '\n':
                if not any(ord(b) > 127 for b in ''.join(buf).encode('utf-8', 'ignore')):
                    out.append(''.join(buf))          # 纯ASCII注释保留
                out.append('\n'); state = 'N'
            else: buf.append(c)
            i += 1
        elif state == 'B':
            buf.append(c)
            if text.startswith('*/', i):
                buf.append(text[i+1])
                joined = ''.join(buf)
                if not any(ord(ch) > 127 for ch in joined):
                    out.append(joined)                 # 纯ASCII块注释保留
                state = 'N'; i += 2
            else: i += 1
        elif state == 'S':
            out.append(c)
            if c == '\\': out.append(text[i+1]); i += 2; continue
            if c == '"': state = 'N'
            i += 1
        elif state == 'C':
            out.append(c)
            if c == '\\': out.append(text[i+1]); i += 2; continue
            if c == "'": state = 'N'
            i += 1
    return ''.join(out)

# 主流程：对每个文件
#   raw = open(p,'rb').read()
#   text = raw.decode('gbk') 或 'utf-8'（尝试顺序：utf-8 → gbk）
#   new = 压缩空行(strip_cn_comments(text))
#   open(p,'w',encoding='utf-8',newline='\n').write(new)
```

## 坑 1：正点原子文件带只读属性

`ls -l` 显示 `r--r--r--` → Python `open(p,'w')` 报 `PermissionError: [Errno 13]`。
**错误的解法（本会话实测失败）：**
- bash 里 `attrib -r path\file`：反斜杠被 MSYS 转义吃掉，`\\` 变成空 → `Parameter format not correct`
- `powershell -NoProfile -Command "... | Where {$_.IsReadOnly}..."`：`$_` 被 bash 当变量展开成空 → 报 `/g/path.IsReadOnly : command not found`

**正确解法：写 .ps1 文件再 -File 执行**

```powershell
# clear-ro.ps1
$dirs = @('G:\...\LED_vscode\USER','G:\...\LED_vscode\SYSTEM','G:\...\LED_vscode\HARDWARE')
Get-ChildItem -Recurse -Path $dirs -Include *.c,*.h | ForEach-Object {
  if ($_.IsReadOnly) { $_.IsReadOnly = $false; Write-Output ("RO-cleared: " + $_.FullName) }
}
```

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File clear-ro.ps1
```

## 坑 2：验证的退出码陷阱

```bash
grep -rlP '[^\x00-\x7F]' USER SYSTEM HARDWARE --include="*.c" --include="*.h" | head -5; echo "grep-exit=$?"
```
`$?` 取的是 **head** 的退出码（永远 0）——这个验证是假的。正确写法用变量捕获：

```bash
files=$(grep -rlP '[^\x00-\x7F]' USER SYSTEM HARDWARE --include="*.c" --include="*.h")
if [ -z "$files" ]; then echo "CLEAN"; else echo "RESIDUE:"; echo "$files"; fi
```

## 验证三板斧（全过才算数）

1. **ninja 全量重编**：`ninja -C build_test`（或 build/Debug）→ 0 error，Memory region 正常输出
2. **非 ASCII 零残留**：上面的变量捕获版 grep，报 CLEAN
3. **人工抽查**：cat 一个核心文件（如 led.c），确认代码完好、纯 ASCII 注释（//PB8）还在、中文注释整段消失

## 副作用与边界

- 文件编码 GBK → UTF-8、换行 CRLF → LF、连续空行压缩
- **Keil 侧独立副本不受影响**：Keil 工程（LED_Keil 目录）的源码还是 GBK 原版，两边互不干扰；若同一目录双用，Keil AC5 默认 GBK 读 UTF-8 会反向乱码
- HALLIB/CORE（ST 官方英文源码）本来就没中文，跳过不处理
- 删除量参考：LED 工程 10 文件、约 380 条中文注释（delay.c 122 / usart.c 91 / sys.c 34+38）

## 关联

- keil2cubevscode 脚本「已知问题第 5 节」（SKILL.md 主文件）
- `references/keil-gcc-porting.md` — 移植三坑（启动文件/CMSIS头/内联汇编），本步骤是其收尾
