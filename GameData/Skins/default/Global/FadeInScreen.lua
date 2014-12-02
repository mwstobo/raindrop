function FadeInA1(frac)
	Obj.SetPosition(ScreenWidth/2, ScreenHeight/4 * frac)
	Obj.SetScale(1, frac)
	return 1
end

function FadeInA2(frac)
	Obj.SetPosition(ScreenWidth/2, ScreenHeight - (ScreenHeight/4) * frac)
	Obj.SetScale(1, frac)
	return 1
end

function invert(f, frac)
	newf = function (frac)
		return f(1 - frac)
	end

	return newf
end

ScreenFade = {}

function ScreenFade.Init()
	Black1 = Object2D()
	Black2 = Object2D()
	
	Black1.Image = "Global/filter.png"
	Black2.Image = "Global/filter.png"
	
	Black1.Centered = 1
	Black2.Centered = 1
	
	Black1.X = ScreenWidth/2
	Black2.X = ScreenWidth/2
	
	Black1.Y = ScreenWidth/4
	Black2.Y = ScreenWidth*3/4
	
	Black1.Alpha = 1
	Black2.Alpha = 1
	
	Black1.Width = ScreenWidth
	Black2.Width = ScreenWidth
	
	Black1.Height = ScreenHeight/2
	Black2.Height = ScreenHeight/2
	Black1.Z = 31
	Black2.Z = 31
	
	Engine:AddTarget(Black1)
	Engine:AddTarget(Black2)
	
	IFadeInA1 = invert(FadeInA1)
	IFadeInA2 = invert(FadeInA2)
end

function ScreenFade.In()
	Engine:StopAnimation(Black1)
	Engine:StopAnimation(Black2)
	Engine:AddAnimation(Black1, "FadeInA1", EaseNone, 0.2, 0)
	Engine:AddAnimation(Black2, "FadeInA2", EaseNone, 0.2, 0)
end

function ScreenFade.Out()
	Engine:StopAnimation(Black1)
	Engine:StopAnimation(Black2)
	Engine:AddAnimation(Black1, "IFadeInA1", EaseNone, 0.2, 0)
	Engine:AddAnimation(Black2, "IFadeInA2", EaseNone, 0.2, 0)
end
