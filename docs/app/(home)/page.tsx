import Link from 'next/link';

export default function HomePage() {
  return (
    <div className="flex flex-col justify-center text-center flex-1 px-4">
      <div className="max-w-2xl mx-auto">
        <h1 className="text-4xl font-bold mb-6 bg-gradient-to-r from-green-600 to-emerald-600 bg-clip-text text-transparent">
          Emerald
        </h1>
        <p className="text-xl text-muted-foreground mb-8">
          A modern systems programming language for performance, safety, and expressiveness
        </p>

        <div className="flex flex-col sm:flex-row gap-4 justify-center mb-12">
          <Link
            href="/docs"
            className="inline-flex items-center justify-center px-6 py-3 bg-primary text-primary-foreground rounded-lg font-medium hover:bg-primary/90 transition-colors"
          >
            Get Started
          </Link>
          <Link
            href="/docs/examples"
            className="inline-flex items-center justify-center px-6 py-3 border border-border rounded-lg font-medium hover:bg-accent transition-colors"
          >
            View Examples
          </Link>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 text-left">
          <div className="p-6 rounded-lg border bg-card">
            <h3 className="font-semibold mb-2">🚀 Performance</h3>
            <p className="text-sm text-muted-foreground">
              Compiles to efficient machine code with zero-cost abstractions
            </p>
          </div>
          <div className="p-6 rounded-lg border bg-card">
            <h3 className="font-semibold mb-2">🛡️ Safety</h3>
            <p className="text-sm text-muted-foreground">
              Built-in memory safety and bounds checking
            </p>
          </div>
          <div className="p-6 rounded-lg border bg-card">
            <h3 className="font-semibold mb-2">🔧 Modern</h3>
            <p className="text-sm text-muted-foreground">
              Clean syntax with powerful language features
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}
