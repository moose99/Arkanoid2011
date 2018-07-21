#include <iostream>
#include <memory>
#include <typeinfo>
#include <map>

#if 0
#include <SFML/Graphics.hpp>

constexpr unsigned int wndWidth{ 800 }, wndHeight{ 600 };

class Entity
{
public:
	bool destroyed{ false };

	virtual ~Entity() {}
	virtual void update() {}
	virtual void draw(sf::RenderWindow& mTarget) {}
};

class Manager
{
private:
	std::vector<std::unique_ptr<Entity>> entities;
	std::map<std::size_t, std::vector<Entity*>> groupedEntities;

public:
	template <typename T, typename... TArgs>
	T& create(TArgs&&... mArgs)
	{
		static_assert(std::is_base_of<Entity, T>::value,
			"`T` must be derived from `Entity`");

		auto uPtr(std::make_unique<T>(std::forward<TArgs>(mArgs)...));
		auto ptr(uPtr.get());
		groupedEntities[typeid(T).hash_code()].emplace_back(ptr);
		entities.emplace_back(std::move(uPtr));

		return *ptr;
	}

	void refresh()
	{
		for (auto& pair : groupedEntities)
		{
			auto& vector(pair.second);

			vector.erase(std::remove_if(std::begin(vector), std::end(vector),
				[](auto mPtr)
			{
				return mPtr->destroyed;
			}),
				std::end(vector));
		}

		entities.erase(std::remove_if(std::begin(entities), std::end(entities),
			[](const auto& mUPtr)
		{
			return mUPtr->destroyed;
		}),
			std::end(entities));
	}

	void clear()
	{
		groupedEntities.clear();
		entities.clear();
	}

	template <typename T>
	auto& getAll()
	{
		return groupedEntities[typeid(T).hash_code()];
	}

	template <typename T, typename TFunc>
	void forEach(const TFunc& mFunc)
	{
		for (auto ptr : getAll<T>()) mFunc(*reinterpret_cast<T*>(ptr));
	}

	void update()
	{
		for (auto& e : entities) e->update();
	}
	void draw(sf::RenderWindow& mTarget)
	{
		for (auto& e : entities) e->draw(mTarget);
	}
};

struct Rectangle
{
	sf::RectangleShape shape;

	float x() const noexcept { return shape.getPosition().x; }
	float y() const noexcept { return shape.getPosition().y; }
	float width() const noexcept { return shape.getSize().x; }
	float height() const noexcept { return shape.getSize().y; }
	float left() const noexcept { return x() - width() / 2.f; }
	float right() const noexcept { return x() + width() / 2.f; }
	float top() const noexcept { return y() - height() / 2.f; }
	float bottom() const noexcept { return y() + height() / 2.f; }
};

struct Circle
{
	sf::CircleShape shape;

	float x() const noexcept { return shape.getPosition().x; }
	float y() const noexcept { return shape.getPosition().y; }
	float radius() const noexcept { return shape.getRadius(); }
	float left() const noexcept { return x() - radius(); }
	float right() const noexcept { return x() + radius(); }
	float top() const noexcept { return y() - radius(); }
	float bottom() const noexcept { return y() + radius(); }
};

class Ball : public Entity, public Circle
{
public:
	static const sf::Color defColor;
	static constexpr float defRadius{ 10.f }, defVelocity{ 8.f };

	sf::Vector2f velocity{ -defVelocity, -defVelocity };

	Ball(float mX, float mY)
	{
		shape.setPosition(mX, mY);
		shape.setRadius(defRadius);
		shape.setFillColor(defColor);
		shape.setOrigin(defRadius, defRadius);
	}

	void update() override
	{
		shape.move(velocity);
		solveBoundCollisions();
	}

	void draw(sf::RenderWindow& mTarget) override { mTarget.draw(shape); }

private:
	void solveBoundCollisions() noexcept
	{
		if (left() < 0)
			velocity.x = defVelocity;
		else if (right() > wndWidth)
			velocity.x = -defVelocity;

		if (top() < 0)
			velocity.y = defVelocity;
		else if (bottom() > wndHeight)
		{
			// If the ball leaves the window towards the bottom,
			// we destroy it.
			destroyed = true;
		}
	}
};

const sf::Color Ball::defColor{ sf::Color::Red };

class Paddle : public Entity, public Rectangle
{
public:
	static const sf::Color defColor;
	static constexpr float defWidth{ 60.f }, defHeight{ 20.f };
	static constexpr float defVelocity{ 8.f };

	sf::Vector2f velocity;

	Paddle(float mX, float mY)
	{
		shape.setPosition(mX, mY);
		shape.setSize({ defWidth, defHeight });
		shape.setFillColor(defColor);
		shape.setOrigin(defWidth / 2.f, defHeight / 2.f);
	}

	void update() override
	{
		processPlayerInput();
		shape.move(velocity);
	}

	void draw(sf::RenderWindow& mTarget) override { mTarget.draw(shape); }

private:
	void processPlayerInput()
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) && left() > 0)
			velocity.x = -defVelocity;
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) &&
			right() < wndWidth)
			velocity.x = defVelocity;
		else
			velocity.x = 0;
	}
};

const sf::Color Paddle::defColor{ sf::Color::Red };

class Brick : public Entity, public Rectangle
{
public:
	static const sf::Color defColorHits1;
	static const sf::Color defColorHits2;
	static const sf::Color defColorHits3;
	static constexpr float defWidth{ 60.f }, defHeight{ 20.f };
	static constexpr float defVelocity{ 8.f };

	// Let's add a field for the required hits.
	int requiredHits{ 1 };

	Brick(float mX, float mY)
	{
		shape.setPosition(mX, mY);
		shape.setSize({ defWidth, defHeight });
		shape.setOrigin(defWidth / 2.f, defHeight / 2.f);
	}

	void update() override
	{
		// Let's alter the color of the brick depending on the
		// required hits.
		if (requiredHits == 1)
			shape.setFillColor(defColorHits1);
		else if (requiredHits == 2)
			shape.setFillColor(defColorHits2);
		else
			shape.setFillColor(defColorHits3);
	}
	void draw(sf::RenderWindow& mTarget) override { mTarget.draw(shape); }
};

const sf::Color Brick::defColorHits1{ 255, 255, 0, 80 };
const sf::Color Brick::defColorHits2{ 255, 255, 0, 170 };
const sf::Color Brick::defColorHits3{ 255, 255, 0, 255 };

template <typename T1, typename T2>
bool isIntersecting(const T1& mA, const T2& mB) noexcept
{
	return mA.right() >= mB.left() && mA.left() <= mB.right() &&
		mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
}

void solvePaddleBallCollision(const Paddle& mPaddle, Ball& mBall) noexcept
{
	if (!isIntersecting(mPaddle, mBall)) return;

	mBall.velocity.y = -Ball::defVelocity;
	mBall.velocity.x =
		mBall.x() < mPaddle.x() ? -Ball::defVelocity : Ball::defVelocity;
}

void solveBrickBallCollision(Brick& mBrick, Ball& mBall) noexcept
{
	if (!isIntersecting(mBrick, mBall)) return;

	// Instead of immediately destroying the brick upon collision,
	// we decrease and check its required hits first.
	--mBrick.requiredHits;
	if (mBrick.requiredHits <= 0) mBrick.destroyed = true;

	float overlapLeft{ mBall.right() - mBrick.left() };
	float overlapRight{ mBrick.right() - mBall.left() };
	float overlapTop{ mBall.bottom() - mBrick.top() };
	float overlapBottom{ mBrick.bottom() - mBall.top() };

	bool ballFromLeft(std::abs(overlapLeft) < std::abs(overlapRight));
	bool ballFromTop(std::abs(overlapTop) < std::abs(overlapBottom));

	float minOverlapX{ ballFromLeft ? overlapLeft : overlapRight };
	float minOverlapY{ ballFromTop ? overlapTop : overlapBottom };

	if (std::abs(minOverlapX) < std::abs(minOverlapY))
		mBall.velocity.x =
		ballFromLeft ? -Ball::defVelocity : Ball::defVelocity;
	else
		mBall.velocity.y = ballFromTop ? -Ball::defVelocity : Ball::defVelocity;
}

class Game
{
private:
	// There now are two additional game states: `GameOver`
	// and `Victory`.
	enum class State
	{
		Paused,
		GameOver,
		InProgress,
		Victory
	};

	static constexpr int brkCountX{ 11 }, brkCountY{ 4 };
	static constexpr int brkStartColumn{ 1 }, brkStartRow{ 2 };
	static constexpr float brkSpacing{ 3.f }, brkOffsetX{ 22.f };

	sf::RenderWindow window{ { wndWidth, wndHeight }, "Arkanoid - 11" };
	Manager manager;

	// SFML offers an easy-to-use font and text class that we
	// can use to display remaining lives and game status.
	sf::Font liberationSans;
	sf::Text textState, textLives;

	State state{ State::GameOver };
	bool pausePressedLastFrame{ false };

	// Let's keep track of the remaning lives in the game class.
	int remainingLives{ 0 };

public:
	Game()
	{
		window.setFramerateLimit(60);

		// We need to load a font from file before using
		// our text objects.
		if (!liberationSans.loadFromFile("calibri.ttf"))
		{
			std::cout << "Font load failure";
			return;
		}

		textState.setFont(liberationSans);
		textState.setPosition(10, 10);
		textState.setCharacterSize(35);
		textState.setFillColor(sf::Color::White);
		textState.setString("Paused");

		textLives.setFont(liberationSans);
		textLives.setPosition(10, 10);
		textLives.setCharacterSize(15);
		textLives.setFillColor(sf::Color::White);
	}

	void restart()
	{
		// Let's remember to reset the remaining lives.
		remainingLives = 3;

		state = State::Paused;
		manager.clear();

		for (int iX{ 0 }; iX < brkCountX; ++iX)
			for (int iY{ 0 }; iY < brkCountY; ++iY)
			{
				float x{ (iX + brkStartColumn) * (Brick::defWidth + brkSpacing) };
				float y{ (iY + brkStartRow) * (Brick::defHeight + brkSpacing) };

				auto& brick(manager.create<Brick>(brkOffsetX + x, y));

				// Let's set the required hits for the bricks.
				brick.requiredHits = 1 + ((iX * iY) % 3);
			}

		manager.create<Ball>(wndWidth / 2.f, wndHeight / 2.f);
		manager.create<Paddle>(wndWidth / 2.f, wndHeight - 50.0f);
	}

	void run()
	{
		while (true)
		{
			window.clear(sf::Color::Black);

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) break;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P))
			{
				if (!pausePressedLastFrame)
				{
					if (state == State::Paused)
						state = State::InProgress;
					else if (state == State::InProgress)
						state = State::Paused;
				}
				pausePressedLastFrame = true;
			}
			else
				pausePressedLastFrame = false;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) restart();

			// If the game is not in progress, do not draw or update
			// game elements and display information to the player.
			if (state != State::InProgress)
			{
				if (state == State::Paused)
					textState.setString("Paused");
				else if (state == State::GameOver)
					textState.setString("Game over!");
				else if (state == State::Victory)
					textState.setString("You won!");

				window.draw(textState);
			}
			else
			{
				// If there are no more balls on the screen, spawn a
				// new one and remove a life.
				if (manager.getAll<Ball>().empty())
				{
					manager.create<Ball>(wndWidth / 2.f, wndHeight / 2.f);

					--remainingLives;
				}

				// If there are no more bricks on the screen,
				// the player won!
				if (manager.getAll<Brick>().empty()) state = State::Victory;

				// If the player has no more remaining lives,
				// it's game over!
				if (remainingLives <= 0) state = State::GameOver;

				manager.update();

				manager.forEach<Ball>([this](auto& mBall)
				{
					manager.forEach<Brick>([&mBall](auto& mBrick)
					{
						solveBrickBallCollision(mBrick, mBall);
					});
					manager.forEach<Paddle>([&mBall](auto& mPaddle)
					{
						solvePaddleBallCollision(mPaddle, mBall);
					});
				});

				manager.refresh();

				manager.draw(window);

				// Update lives string and draw it.
				textLives.setString("Lives: " + std::to_string(remainingLives));

				window.draw(textLives);
			}

			window.display();
		}
	}
};

int main()
{
	Game game;
	game.restart();
	game.run();
	return 0;
}

#else

#include<string>
#include <ostream>
#include "SDL.h"
#include "SDL_ttf.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

/**
* Log an SDL error with some error message to the output stream of our choice
* @param os The output stream to write the message to
* @param msg The error message to write, format will be msg error: SDL_GetError()
*/
void logSDLError( std::ostream &os, const std::string &msg )
{
	os << msg << " error: " << SDL_GetError() << std::endl;
}

/**
* Render the message we want to display to a texture for drawing
* @param message The message we want to display
* @param fontFile The font we want to use to render the text
* @param color The color we want the text to be
* @param fontSize The size we want the font to be
* @param renderer The renderer to load the texture in
* @return An SDL_Texture containing the rendered message, or nullptr if something went wrong
*/
SDL_Texture* renderText( const std::string &message, const std::string &fontFile,
	SDL_Color color, int fontSize, SDL_Renderer *renderer )
{
	//Open the font
	TTF_Font *font = TTF_OpenFont( fontFile.c_str(), fontSize );
	if (font == nullptr)
	{
		logSDLError( std::cout, "TTF_OpenFont" );
		return nullptr;
	}
	//We need to first render to a surface as that's what TTF_RenderText
	//returns, then load that surface into a texture
	SDL_Surface *surf = TTF_RenderText_Blended( font, message.c_str(), color );
	if (surf == nullptr)
	{
		TTF_CloseFont( font );
		logSDLError( std::cout, "TTF_RenderText" );
		return nullptr;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface( renderer, surf );
	if (texture == nullptr)
	{
		logSDLError( std::cout, "CreateTexture" );
	}
	//Clean up the surface and font
	SDL_FreeSurface( surf );
	TTF_CloseFont( font );
	return texture;
}

/**
* Draw an SDL_Texture to an SDL_Renderer at some destination rect
* taking a clip of the texture if desired
* @param tex The source texture we want to draw
* @param ren The renderer we want to draw to
* @param dst The destination rectangle to render the texture to
* @param clip The sub-section of the texture to draw (clipping rect)
*		default of nullptr draws the entire texture
*/
void renderTexture( SDL_Texture *tex, SDL_Renderer *ren, SDL_Rect dst,
	SDL_Rect *clip = nullptr )
{
	SDL_RenderCopy( ren, tex, clip, &dst );
}

/**
* Draw an SDL_Texture to an SDL_Renderer at position x, y, preserving
* the texture's width and height and taking a clip of the texture if desired
* If a clip is passed, the clip's width and height will be used instead of
*	the texture's
* @param tex The source texture we want to draw
* @param ren The renderer we want to draw to
* @param x The x coordinate to draw to
* @param y The y coordinate to draw to
* @param clip The sub-section of the texture to draw (clipping rect)
*		default of nullptr draws the entire texture
*/
void renderTexture( SDL_Texture *tex, SDL_Renderer *ren, int x, int y,
	SDL_Rect *clip = nullptr )
{
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	if (clip != nullptr)
	{
		dst.w = clip->w;
		dst.h = clip->h;
	}
	else
	{
		SDL_QueryTexture( tex, NULL, NULL, &dst.w, &dst.h );
	}
	renderTexture( tex, ren, dst, clip );
}

void fill_circle( SDL_Renderer *renderer, int cx, int cy, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a )
{
	// Note that there is more to altering the bitrate of this 
	// method than just changing this value.  See how pixels are
	// altered at the following web page for tips:
	//   http://www.libsdl.org/intro.en/usingvideo.html
	static const int BPP = 4;

	//double ra = (double)radius;

	for (double dy = 1; dy <= radius; dy += 1.0)
	{
		// This loop is unrolled a bit, only iterating through half of the
		// height of the circle.  The result is used to draw a scan line and
		// its mirror image below it.

		// The following formula has been simplified from our original.  We
		// are using half of the width of the circle because we are provided
		// with a center and we need left/right coordinates.

		double dx = floor( sqrt( (2.0 * radius * dy) - (dy * dy) ) );
		int x = cx - dx;
		SDL_SetRenderDrawColor( renderer, r, g, b, a );
		SDL_RenderDrawLine( renderer, cx - dx, cy + dy - radius, cx + dx, cy + dy - radius );
		SDL_RenderDrawLine( renderer, cx - dx, cy - dy + radius, cx + dx, cy - dy + radius );

		// Grab a pointer to the left-most pixel for each half of the circle
		/*Uint8 *target_pixel_a = (Uint8 *)surface->pixels + ((int)(cy + r - dy)) * surface->pitch + x * BPP;
		Uint8 *target_pixel_b = (Uint8 *)surface->pixels + ((int)(cy - r + dy)) * surface->pitch + x * BPP;
		for (; x <= cx + dx; x++)
		{
		*(Uint32 *)target_pixel_a = pixel;
		*(Uint32 *)target_pixel_b = pixel;
		target_pixel_a += BPP;
		target_pixel_b += BPP;
		}*/
		/*
		// sleep for debug
		SDL_RenderPresent(gRenderer);
		std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
		*/
	}
}

int main( int, char** )
{
	// init SDL
	if (SDL_Init( SDL_INIT_EVERYTHING ) != 0)
	{
		logSDLError( std::cout, "SDL_Init" );
		return 1;
	}

	// init TTF system
	if (TTF_Init() != 0)
	{
		logSDLError( std::cout, "TTF_Init" );
		SDL_Quit();
		return 1;
	}

	// Create window and renderer
	SDL_Window *window = SDL_CreateWindow( "ARKANOID", 100, 100, SCREEN_WIDTH,
		SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	if (window == nullptr)
	{
		logSDLError( std::cout, "CreateWindow" );
		SDL_Quit();
		return 1;
	}
	SDL_Renderer *renderer = SDL_CreateRenderer( window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if (renderer == nullptr)
	{
		logSDLError( std::cout, "CreateRenderer" );
		SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	// Load bitmap
	std::string imagePath = "hello.bmp";
	SDL_Surface *bmp = SDL_LoadBMP( imagePath.c_str() );
	if (bmp == nullptr)
	{
		SDL_DestroyRenderer( renderer );
		SDL_DestroyWindow( window );
		std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	// upload image to renderer
	SDL_Texture *tex = SDL_CreateTextureFromSurface( renderer, bmp );
	SDL_FreeSurface( bmp );
	if (tex == nullptr)
	{
		SDL_DestroyRenderer( renderer );
		SDL_DestroyWindow( window );
		std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_Color color = { 0, 0, 255, 255 };
	SDL_Texture *image = renderText( "TTF fonts are cool!", "calibri.ttf",
		color, 16, renderer );
	if (image == nullptr)
	{
		SDL_DestroyRenderer( renderer );
		SDL_DestroyWindow( window );
		TTF_Quit();
		SDL_Quit();
		return 1;
	}

	//Get the texture w/h so we can center it in the screen
	int iW, iH;
	SDL_QueryTexture( image, NULL, NULL, &iW, &iH );
	int x = SCREEN_WIDTH / 2 - iW / 2;
	int y = SCREEN_HEIGHT / 2 - iH / 2;


	//e is an SDL_Event variable we've declared before entering the main loop
	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent( &e ))
		{
			//If user closes the window, presses escape
			if (e.type == SDL_QUIT ||
				(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) )
			{
				quit = true;
			}
		}

		/* Select the color for drawing. It is set to red here. */
		SDL_SetRenderDrawColor( renderer, 255, 0, 0, 255 );
		//First clear the renderer with the draw color
		SDL_RenderClear( renderer );

		//Draw the texture
		SDL_RenderCopy( renderer, tex, NULL, NULL );
		//We can draw our message as we do any other texture, since it's been
		//rendered to a texture
		renderTexture( image, renderer, x, y );
		
		//Render green filled quad and circle
		SDL_Rect fillRect = { SCREEN_WIDTH / 8, SCREEN_HEIGHT / 8, SCREEN_WIDTH / 8, SCREEN_HEIGHT / 8 };
		SDL_SetRenderDrawColor( renderer, 0x00, 0xff, 0x00, 0xFF );
		SDL_RenderFillRect( renderer, &fillRect );
		fill_circle( renderer, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2+50, 20, 0, 255, 0, 255 );
		

		//Update the screen
		SDL_RenderPresent( renderer );
	}

	// cleanup
	SDL_DestroyTexture( tex );
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );
	SDL_Quit();

	return 0;
}

#endif