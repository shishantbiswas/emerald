# Emerald Language Server

A Language Server Protocol (LSP) implementation for the Emerald programming language.

## Features

- Syntax highlighting
- Code completion
- Hover information
- Document symbols
- Go to definition
- Diagnostics and error reporting

## Installation

### Prerequisites

- Node.js 16+
- npm or yarn

### Building

```bash
# Install dependencies
npm install

# Build the server
npm run build

# Run tests
npm test
```

### VS Code Extension

The LSP server can be integrated with VS Code using the Emerald VS Code extension.

## Architecture

- `src/server.ts` - Main LSP server implementation
- `src/client.ts` - VS Code client integration (future)
- `src/emerald.ts` - Emerald language utilities

## Language Features

### Supported LSP Capabilities

- **textDocumentSync**: Incremental document synchronization
- **completionProvider**: Code completion for keywords and functions
- **hoverProvider**: Hover information for symbols
- **documentSymbolProvider**: Document symbol navigation
- **definitionProvider**: Go to definition support

### Emerald Language Support

- Function declarations (`function`)
- Variable declarations (`let`)
- Control flow (`if`, `for`)
- Built-in functions (`print`)

## Development

### Running in Development

```bash
# Watch mode
npm run watch

# With debugging
npm run build && node out/server.js
```

### Testing

```bash
npm test
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

MIT License - see LICENSE file for details.