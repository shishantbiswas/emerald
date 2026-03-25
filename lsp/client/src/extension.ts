import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';

import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
  // The server is implemented in node
  const serverModule = context.asAbsolutePath(
    path.join('..', 'out', 'server.js')
  );

  // If the extension is launched in debug mode then the debug server options are used
  // Otherwise the run options are used
  const serverOptions: ServerOptions = {
    // For now, run the server as a separate process
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: {
      module: serverModule,
      transport: TransportKind.ipc,
      options: { execArgv: ['--nolazy', '--inspect=6009'] }
    }
  };

  // Options to control the language client
  const clientOptions: LanguageClientOptions = {
    // Register the server for emerald documents
    documentSelector: [{ scheme: 'file', language: 'emerald' }],
    synchronize: {
      // Notify the server about file changes to '.emerald' files contained in the workspace
      fileEvents: workspace.createFileSystemWatcher('**/*.em'),
    },
  };

  // Create the language client and start the client.
  client = new LanguageClient(
    'emeraldLanguageServer',
    'Emerald Language Server',
    serverOptions,
    clientOptions
  );

  // Start the client. This will also launch the server
  client.start();
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}