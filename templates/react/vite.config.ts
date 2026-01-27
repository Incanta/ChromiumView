import { defineConfig, Plugin, PluginOption } from "vite";
import react from "@vitejs/plugin-react-swc";
import tailwindcss from "@tailwindcss/vite";
import { resolve, basename, dirname, relative } from "path";
import { readdirSync, statSync, existsSync, writeFileSync, mkdirSync, rmSync, readFileSync } from "fs";

const srcDir = resolve(__dirname, "src");
const viewsDir = resolve(srcDir, "views");
const generatedDir = resolve(viewsDir, ".generated");

/**
 * Generates a minimal HTML file for a TSX entry point
 */
function generateHtml(name: string, tsxPath: string): string {
  // Convert name to title case for the page title
  const title = name
    .split("-")
    .map(word => word.charAt(0).toUpperCase() + word.slice(1))
    .join(" ");

  // Use relative path from .generated to src/views
  const relativeTsxPath = "../" + relative(viewsDir, tsxPath).replace(/\\/g, "/");

  return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>${title}</title>
  <style>
    html, body, #root {
      width: 100%;
      height: 100%;
      overflow: hidden;
      background: transparent;
    }
  </style>
</head>
<body>
  <div id="root"></div>
  <script type="module" src="${relativeTsxPath}"></script>
</body>
</html>
`;
}

/**
 * Find all TSX entry points in the views directory
 */
function getTsxEntryPoints(dir: string, prefix = ""): { name: string; tsxPath: string }[] {
  const entries: { name: string; tsxPath: string }[] = [];

  if (!existsSync(dir)) {
    return entries;
  }

  const files = readdirSync(dir);

  for (const file of files) {
    const fullPath = resolve(dir, file);
    const stat = statSync(fullPath);

    if (stat.isDirectory()) {
      // Skip directories starting with _
      if (file.startsWith("_")) {
        continue;
      }
      entries.push(...getTsxEntryPoints(fullPath, prefix ? `${prefix}/${file}` : file));
    } else if (file.endsWith(".tsx") && !file.startsWith("_") && !file.endsWith(".stories.tsx")) {
      const name = prefix
        ? `${prefix}/${file.replace(".tsx", "")}`
        : file.replace(".tsx", "");
      entries.push({ name, tsxPath: fullPath });
    }
  }

  return entries;
}

/**
 * Plugin to generate HTML files from TSX entry points
 */
function generateHtmlPlugin(): Plugin {
  return {
    name: "generate-html-entries",
    buildStart() {
      // Clean and recreate generated directory
      if (existsSync(generatedDir)) {
        rmSync(generatedDir, { recursive: true });
      }
      mkdirSync(generatedDir, { recursive: true });

      // Generate HTML files for each TSX entry point in views/
      const entries = getTsxEntryPoints(viewsDir);
      for (const { name, tsxPath } of entries) {
        const htmlContent = generateHtml(basename(name), tsxPath);
        const htmlPath = resolve(generatedDir, `${name}.html`);
        const htmlDir = dirname(htmlPath);

        if (!existsSync(htmlDir)) {
          mkdirSync(htmlDir, { recursive: true });
        }

        writeFileSync(htmlPath, htmlContent);
      }
    },
  };
}

/**
 * Plugin to remove crossorigin attribute from HTML files after build.
 * This is necessary for file:// URLs to work properly with ES modules in CEF.
 * The crossorigin attribute triggers CORS checks which fail for local files.
 */
function removeCrossoriginPlugin(): Plugin {
  return {
    name: "remove-crossorigin",
    writeBundle(options) {
      const outDir = options.dir || resolve(__dirname, "../View");
      
      function processDir(dir: string) {
        if (!existsSync(dir)) return;
        
        const files = readdirSync(dir);
        for (const file of files) {
          const fullPath = resolve(dir, file);
          const stat = statSync(fullPath);
          
          if (stat.isDirectory()) {
            processDir(fullPath);
          } else if (file.endsWith(".html")) {
            let content = readFileSync(fullPath, "utf-8");
            // Remove crossorigin attribute from script and link tags
            const originalContent = content;
            content = content.replace(/ crossorigin(?:="[^"]*")?/g, "");
            if (content !== originalContent) {
              writeFileSync(fullPath, content);
              console.log(`[remove-crossorigin] Removed crossorigin from: ${file}`);
            }
          }
        }
      }
      
      processDir(outDir);
    },
  };
}

/**
 * Get entry points from generated HTML files
 */
function getGeneratedEntryPoints(): Record<string, string> {
  const entries: Record<string, string> = {};

  function scanDir(dir: string, prefix = "") {
    if (!existsSync(dir)) return;

    const files = readdirSync(dir);
    for (const file of files) {
      const fullPath = resolve(dir, file);
      const stat = statSync(fullPath);

      if (stat.isDirectory()) {
        scanDir(fullPath, prefix ? `${prefix}/${file}` : file);
      } else if (file.endsWith(".html")) {
        const name = prefix
          ? `${prefix}/${file.replace(".html", "")}`
          : file.replace(".html", "");
        entries[name] = fullPath;
      }
    }
  }

  scanDir(generatedDir);
  return entries;
}

/**
 * Plugin to rewrite URLs without .html extension to their .html counterparts.
 * This allows `/sample-hud` to serve `sample-hud.html`.
 */
function htmlFallbackPlugin(): Plugin {
  return {
    name: "html-fallback",
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        if (!req.url) return next();

        // Parse the URL path (remove query string)
        const urlPath = req.url.split("?")[0];

        // Skip if it already has an extension or is a special Vite path
        if (
          urlPath.includes(".") ||
          urlPath.startsWith("/@") ||
          urlPath.startsWith("/__") ||
          urlPath === "/"
        ) {
          return next();
        }

        // Check if a generated HTML file exists for this path
        const htmlPath = resolve(generatedDir, urlPath.slice(1) + ".html");
        if (existsSync(htmlPath)) {
          req.url = urlPath + ".html" + (req.url.includes("?") ? "?" + req.url.split("?")[1] : "");
        }

        next();
      });
    },
  };
}

// Pre-generate HTML files so we can get entry points
// This runs at config load time
if (!existsSync(generatedDir)) {
  mkdirSync(generatedDir, { recursive: true });
}
const entries = getTsxEntryPoints(viewsDir);
for (const { name, tsxPath } of entries) {
  const htmlContent = generateHtml(basename(name), tsxPath);
  const htmlPath = resolve(generatedDir, `${name}.html`);
  const htmlDir = dirname(htmlPath);

  if (!existsSync(htmlDir)) {
    mkdirSync(htmlDir, { recursive: true });
  }

  writeFileSync(htmlPath, htmlContent);
}

const entryPoints = getGeneratedEntryPoints();

export default defineConfig(({ command }) => {
  const isServing = command === "serve";

  const plugins: PluginOption[] = [
    generateHtmlPlugin(),
    htmlFallbackPlugin(),
    tailwindcss(),
    react(),
    // Remove crossorigin attribute from built HTML files for file:// URL compatibility
    removeCrossoriginPlugin(),
  ];

  return {
    plugins,
    root: isServing ? viewsDir : generatedDir,
    // Use relative paths for deployment
    base: isServing ? "" : "./",
    // Ensure only one copy of React is used
    resolve: {
      dedupe: ["react", "react-dom"],
    },
    server: {
      // Configure HMR for embedded browser contexts
      // hmr: {
      //   protocol: "ws",
      //   host: "localhost",
      // },
      // fs: {
      //   // Allow serving files from the parent ViewSource directory
      //   allow: [__dirname, generatedDir, srcDir],
      // },
    },
    build: {
      minify: false,
      outDir: resolve(__dirname, "../View"),
      emptyOutDir: true,
      rollupOptions: {
        input: entryPoints,
        output: {
          entryFileNames: "assets/[name]-[hash].js",
          chunkFileNames: "assets/[name]-[hash].js",
          assetFileNames: "assets/[name]-[hash][extname]",
          // Ensure React is bundled together to avoid shared internals issues
          manualChunks: undefined,
        },
      },
    },
    // Optimize deps to pre-bundle React properly
    optimizeDeps: {
      include: ["react", "react-dom"],
    },
  }
});
