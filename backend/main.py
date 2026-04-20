from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from backend.routes.analysis import router

app = FastAPI(title="Oracle -- Live Win Probability Engine", version="0.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://localhost:5174", "http://localhost:5175", "http://localhost:5176", "http://localhost:5177"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(router)


@app.get("/health")
def health():
    return {"status": "ok", "service": "oracle"}


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("backend.main:app", host="0.0.0.0", port=8002, reload=True)
