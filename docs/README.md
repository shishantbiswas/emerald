# Emerald Documentation

This is the official documentation website for the **Emerald Programming Language**, built with [Fumadocs](https://fumadocs.dev) and Next.js.

Emerald is a modern systems programming language designed for performance, safety, and expressiveness. This documentation site provides comprehensive guides, examples, and API references.

## Development

### Prerequisites

- Node.js 18+
- npm, pnpm, or bun

### Getting Started

```bash
# Install dependencies
npm install
# or
pnpm install
# or
bun install

# Run development server
npm run dev
# or
pnpm dev
# or
bun dev
```

Open [http://localhost:3000](http://localhost:3000) to view the documentation.

### Build for Production

```bash
# Build the documentation site
npm run build

# Preview the production build
npm run start
```

## Project Structure

- `content/docs/` - Documentation content in MDX format
- `app/` - Next.js application routes and components
- `components/` - Reusable UI components
- `lib/` - Utility functions and configurations

### Key Files

- `lib/source.ts` - Content source configuration for Fumadocs
- `lib/layout.shared.tsx` - Shared layout options
- `source.config.ts` - MDX and content processing configuration

## Documentation Content

The documentation is organized in the `content/docs/` directory:

- `index.mdx` - Homepage and overview
- `installation.mdx` - Installation guide
- `language.mdx` - Language syntax and features
- `examples.mdx` - Code examples

## Contributing

To contribute to the documentation:

1. Make changes to the MDX files in `content/docs/`
2. Test locally with `npm run dev`
3. Ensure proper formatting with `npm run format`
4. Check for linting issues with `npm run lint`

## Deployment

The site is configured for static export and can be deployed to any static hosting service like Vercel, Netlify, or GitHub Pages.

```bash
# Build for static export
npm run build

# The static files will be in the `out/` directory
```

## Learn More

- [Emerald Programming Language](https://github.com/your-org/emerald) - Main repository
- [Fumadocs Documentation](https://fumadocs.dev) - Framework documentation
- [Next.js Documentation](https://nextjs.org/docs) - Next.js guides
